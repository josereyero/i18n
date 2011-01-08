<?php 
/**
 * @file
 *   API for internationalization strings
 */

/**
 * Textgroup handler for i18n_string API
 */
class i18n_string_default {
  // Text group name
  public $textgroup;
  // Cached or preloaded translations
  public $translations;
  // Context to locale id map
  protected $string_map;
  // String formats
  protected $string_format;
  /**
   * Class constructor
   */
  public function __construct($textgroup) {
    $this->textgroup = $textgroup;
  }
  /**
   * Build string object
   */
  public function build_string($context, $source, $options = array()) {
    // Set at least the default language to translate to
    $string = new Stdclass();
    $string->textgroup = $this->textgroup;
    $string->source = $source;
    if (is_array($context)) {
      $string->context = implode(':', $context);
      $parts = $context;
    }
    else {
      $string->context = $context;
      // Split the name in four parts, remaining elements will be in the last one
      // Now we take out 'textgroup'. Context will be the remaining string
      $parts = explode(':', $context);
    }
    // Location will be the full string name
    $string->location = $this->textgroup . ':' . $string->context; 
    $string->type = array_shift($parts);
    $string->objectid = $parts ? array_shift($parts) : '';
    $string->objectkey = (int)$string->objectid;
    // Ramaining elements glued again with ':'
    $string->property = $parts ? implode(':', $parts) : '';
    // Get elements from the cache
    $this->cache_get($string);
    return $string; 
  }
  /**
   * Check if input format is allowed for translation 
   *
   * @param $format
   *   Input format key or NULL if not format (will be allowed)
   */
  public static function allowed_format($format = NULL) {
    $allowed_formats = variable_get('i18n_string_allowed_formats', array(variable_get('filter_default_format', 1)));
    return !isset($format) || in_array($format, $allowed_formats);
  }
  /**
   * Add source string to the locale tables for translation.
   *
   * It will also add data into i18n_strings table for faster retrieval and indexing of groups of strings.
   * Some string context doesn't have a numeric oid (I.e. content types), it will be set to zero.
   *
   * This function checks for already existing string without context for this textgroup and updates it accordingly.
   * It is intended for backwards compatibility, using already created strings.
   *
   * @param $i18nstring
   *   String object
   * @param $format
   *   Input format, for strings that will go through some filter
   * @return
   *   Update status.
   */
  protected function add_string($i18nstring, $options = array()) {
    $options += array('watchdog' => TRUE);
    // Default return status if nothing happens
    $status = -1;
    $source = NULL;
    // The string may not be allowed for translation depending on its format.
    if (!$this->check_string($i18nstring, $options)) {
      // The format may have changed and it's not allowed now, delete the source string
      return $this->remove_string($i18nstring, $options);
    }
    elseif ($source = $this->get_source($i18nstring)) {
      $i18nstring->lid = $source->lid;
      if ($source->source != $i18nstring->source || $source->location != $i18nstring->location) {
        // String has changed, mark translations for update
        $status = $this->save_source($source);
        db_update('locales_target')
          ->fields(array('i18n_status' => I18N_STRING_STATUS_UPDATE))
          ->condition('lid', $source->lid)
          ->execute();
      }
      elseif (empty($source->version)) {
        // When refreshing strings, we've done version = 0, update it
        $this->save_source($i18nstring);
      }
    }
    else {
      // We don't have the source object, create it
      $status = $this->save_source($i18nstring);
    }
    // Make sure we have i18n_string part, create or update
    // This will also create the source object if doesn't exist
    $this->save_string($i18nstring);
  
    if ($options['watchdog']) {
      $params = $this->string_params($i18nstring);
      switch ($status) {
        case SAVED_UPDATED:
          watchdog('i18n_string', 'Updated string %location for textgroup %textgroup: %string', $params);
          break;
        case SAVED_NEW:
          watchdog('i18n_string', 'Created string %location for text group %textgroup: %string', $params);
          break;
      }     
    }  
    return $status;
  }
  /**
   * Set string object into cache
   */
  protected function cache_set($string) {
    if (!empty($string->lid)) {
      $this->string_map[$string->context] = $string->lid;
      if (isset($string->format)) {
        $this->string_format[$string->lid] = $string->format;
      }
      if (isset($string->translation)) {
        $this->translations[$string->language][$string->lid] = $string->translation;
      }      
    }
    elseif (isset($string->lid)) {
      // It seems we don't have a source string
      $this->string_map[$string->context] = FALSE;
    }
  }
  
  /**
   * Get translation from cache
   */
  protected function cache_get($string) {
    if (!isset($string->lid) && isset($this->string_map[$string->context])) {
      $string->lid = $this->string_map[$string->context];
    }
    if (!empty($string->lid)) {
      if (isset($this->string_format[$string->lid])) {
        $string->format = $this->string_format[$string->lid];
      }
      if (!empty($string->language) && isset($this->translations[$string->language][$string->lid])) {
        $string->translation = $this->translations[$string->language][$string->lid];
      }
    }
    elseif (isset($string->lid)) {
      // We don't have a source string, lid == FALSE
      if (!empty($string->language)) {
        $string->translation = FALSE;
      }
    }
  }  
  /**
   * Check if string is ok for translation
   */
  protected static function check_string($i18nstring, $options = array()) {
    $options += array('messages' => FALSE, 'watchdog' => TRUE);
    if (!empty($i18nstring->format) && !self::allowed_format($i18nstring->format)) {
      // This format is not allowed, so we remove the string, in this case we produce a warning
      $params = self::string_params($i18nstring);
      drupal_set_message(t('The string %location for textgroup %textgroup is not allowed for translation because of its input format.', $params), 'warning');
      return FALSE;
    }
    else {
      return TRUE;
    }
  }

  /**
   * Get source string provided a string object.
   *
   * @param $context
   *   Context string or object.
   * @return
   *   Context object if it exists.
   */
  protected function get_source($i18nstring) {
    // Search the database using lid if we've got it or textgroup, context otherwise
    $query = db_select('locales_source', 's')->fields('s');
    if (!empty($i18nstring->lid)) {
      $query->condition('s.lid', $i18nstring->lid);
    }
    else {
      $query->condition('s.textgroup', $this->textgroup);
      $query->condition('s.context', $i18nstring->context);
    }
    // Speed up the query, we just need one row
    $source = $query->range(0, 1)->execute()->fetchObject();
    // Update cached map
    $this->string_map[$i18nstring->context] = $source ? $source->lid : FALSE;
    return $source;
  }

  /**
   * Get translation from the database. Full object with input format.
   *
   * This one doesn't return anything if we don't have the full i18n strings data there
   * to prevent missing data resulting in missing input formats
   */
  protected function get_translation($i18nstring) {
    // First, populate available data from the cache
    $this->cache_get($i18nstring);
    
    if (isset($i18nstring->translation)) {
      // Which doesn't mean we've got a translation, only that we've got the result cached
      $translation = $i18nstring;
    }
    else {
      $translation = $this->load_translation($i18nstring);
      if (!$translation) {
        // No source, no translation     
        $translation = $i18nstring;
        $translation->translation = FALSE;
      }
      $this->cache_set($translation);
    }
    // Return the full object if we've got a translation
    return $translation->translation !== FALSE ? $translation : NULL; 
  }
  
  /**
   * Load translation from db
   * 
   * @todo Optimize when we've already got the source string
   */
  protected static function load_translation($i18nstring) {
    $query = self::query_string($i18nstring);
    $query->leftJoin('locales_target', 't', 's.lid = t.lid');
    $query->fields('t', array('translation', 'i18n_status'));
    $query->condition('t.language', $i18nstring->language);    
    // Speed up the query, we just need one row
    if (empty($i18nstring->multiple)) {
      $query->range(0, 1);
    }
    $translations = $query->execute()->fetchAll();

    foreach ($translations as $translation) {
      $translation->translation = isset($translation->translation) ? $translation->translation : FALSE;
      $translation->language = $i18nstring->language;
    }
    if (empty($i18nstring->multiple)) {
      return reset($translations);
    } 
    else {
      return $translations;
    } 
  }

  /**
   * Get string source object
   *
   * @param $context
   *   Context string or object.
   *
   * @return
   *   - Translation string as object if found.
   *   - FALSE if no translation
   *
   */
  protected static function get_string($context) {
    return self::query_string($context)->execute()->fetchObject();
  }
  
  /**
   * Build query for i18n_strings table
   */
  protected static function query_string($context) {
    // Search the database using lid if we've got it or textgroup, context otherwise
    $query = db_select('i18n_strings', 's')->fields('s');
    if (!empty($context->lid)) {
      $query->condition('s.lid', $context->lid);
    }
    else {
      $query->condition('s.textgroup', $context->textgroup);
      if (empty($context->multiple)) {
        $query->condition('s.context', $context->context);
      }
      else {
        // Query multiple strings
        foreach (array('type', 'objectid', 'property') as $field) {
          if (!empty($context->$field)) {
            $query->condition('s.' . $field, $context->$field);
          }
        }
      }
    }
    return $query;  
  }

  /**
   * Remove source and translations for user defined string.
   *
   * Though for most strings the 'name' or 'string id' uniquely identifies that string,
   * there are some exceptions (like profile categories) for which we need to use the
   * source string itself as a search key.
   *
   * @param $context
   *   Textgroup and location glued with ':'.
   * @param $string
   *   Optional source string (string in default language).
   */
  public function remove($context, $string = NULL, $options = array()) {
    $options += array('messages' => TRUE);
    $i18nstring = $this->build_string($context, $string, $options);
    $status = $this->remove_string($i18nstring, $options);
    if ($options['messages'] && $status === SAVED_DELETED) {
      drupal_set_message(t('Deleted string %location for textgroup %textgroup: %string', $this->string_params($i18nstring)));
    }
    return $this;
  }

  /**
   * Remove string object.
   */
  public function remove_string($i18nstring, $options = array()) {
    $options += array('watchdog' => TRUE);
    if ($source = $this->get_source($i18nstring)) {
      db_delete('locales_target')->condition('lid', $source->lid)->execute();
      db_delete('i18n_strings')->condition('lid', $source->lid)->execute();
      db_delete('locales_source')->condition('lid', $source->lid)->execute();
      if ($options['watchdog']) {
        watchdog('i18n_string', 'Deleted string %location for textgroup %textgroup: %string', $this->string_params($i18nstring));
      } 
      return SAVED_DELETED;
    }
  }
  /**
   * Save / update string object
   *
   * There seems to be a race condition sometimes so skip errors, #277711
   * 
   * @param $string
   *   Full string object to be saved
   * @param $source
   *   Source string object
   */
  protected function save_string($string, $update = FALSE) {
    if (empty($string->lid)) {
      if ($source = $this->get_source($string)) {
        $string->lid = $source->lid;
      }
      else {
        // Create source string so we get an lid
        $this->save_source($string);
      }
    }
    if (!isset($string->objectkey)) {
      $string->objectindex = (int)$string->objectid;
    }
    if (!isset($string->format)) {
      $string->format = 0;
    }
    $status = db_merge('i18n_strings')
      ->key(array('lid' => $string->lid))
      ->fields(array(
          'textgroup' => $string->textgroup,
          'context' => $string->context,
          'objectid' => $string->objectid,
          'type' => $string->type,
          'property' => $string->property,
          'objectindex' => $string->objectkey,
          'format' => $string->format,
      ))
      ->execute();
    return $status;
  }

  /**
   * Save source string (create / update)
   */
  protected static function save_source($source) {
    if (empty($source->version)) {
      $source->version = 1;
    }
    return drupal_write_record('locales_source', $source, !empty($source->lid) ? 'lid' : array());
  }
  /**
   * Get message parameters from context and string.
   */
  protected static function string_params($context) {
    return array(
      '%location' => $context->location,
      '%textgroup' => $context->textgroup,
      '%string' => isset($context->source) ? $context->source : t('[empty string]'),
    );
  }
  /**
   * Translate source string
   */
  public function translate($context, $string, $options = array()) {
    $i18nstring = $this->build_string($context, $string, $options);
    return $this->translate_string($i18nstring, $options);
  }
  /**
   * Translate array of source strings
   */
  public function multiple_translate($context, $strings, $options = array()) {
    $i18nstring = $this->build_string($context, NULL, $options);
    // Set the array of keys on the placeholder field
    foreach (array('type', 'objectid', 'property') as $field) {
      if ($i18nstring->$field === '*') {
        $i18nstring->$field = array_keys($strings);
        $property = $field;
      }
    }
    $i18nstring->language = isset($options['langcode']) ? $options['langcode'] : i18n_langcode();
    $translations = $this->multiple_get_translation($i18nstring);
    // Remap translations using index field
    $result = $strings;
    foreach ($translations as $translation) {
      $index = $translation->$property;
      $translation->source = $strings[$index];
      $result[$index] = $translation;
      unset($strings[$index]);
    }
    // Fill in remaining strings for consistency, l10n_client, etc..
    foreach ($strings as $index => $string) {
      $translation = clone $i18nstring;
      $translation->$property = $index;
      $translation->source = $strings[$index];
      $result[$index] = $translation;
    }
    return $result;
  }
  /**
   * Get multiple translations with the available key
   */
  public function multiple_get_translation($i18nstring) {
    $i18nstring->multiple = TRUE;
    $translations = $this->load_translation($i18nstring);
    foreach ($translations as $index => $translation) {
      $this->cache_set($translation);
      if ($translation->translation === FALSE) {
        unset($translations[$index]);
      }
    }
    return $translations;
  }
  
  /**
   * Translate string object
   * 
   * @param $i18nstring
   *   String object
   * @param $options
   *   Array with aditional options
   */
  protected function translate_string($i18nstring, $options = array()) {
    $i18nstring->language = isset($options['langcode']) ? $options['langcode'] : i18n_langcode();
    // Search for existing translation (result will be cached in this function call)
    if ($translation = $this->get_translation($i18nstring)) {
      return $translation;
    }
    else {
      return $i18nstring;
    }
  }
  /**
   * Update / create translation source for user defined strings.
   *
   * @param $name
   *   Textgroup and location glued with ':'.
   * @param $string
   *   Source string in default language. Default language may or may not be English.
   * @param $options
   *   Array with additional options:
   *   - 'format', String format if the string has input format
   *   - 'messages', Whether to print out status messages
   */
  public function update($context, $string, $options = array()) {
    $options += array('format' => 0, 'messages' => TRUE, 'watchdog' => TRUE);
    $i18nstring = $this->build_string($context, $string, $options);
    $i18nstring->format = $options['format'];
    if (!$this->check_string($i18nstring, $options)) {
      $this->remove($context, $string, $options);
      $status = SAVED_DELETED;
    }
    else {
      $status = $this->update_string($i18nstring, $options);
    }

    if ($options['messages']) {
      $params = $this->string_params($i18nstring);
      switch ($status) {
        case SAVED_UPDATED:
          drupal_set_message(t('Updated string %location for textgroup %textgroup: %string', $params));
          break;
        case SAVED_NEW:
          drupal_set_message(t('Created string %location for text group %textgroup: %string', $params));
          break;
      }     
    }
    return $this;
  }
  /**
   * Update / create / remove string.
   *
   * @param $name
   *   String context.
   * @pram $string
   *   New value of string for update/create. May be empty for removing.
   * @param $format
   *   Input format, that must have been checked against allowed formats for translation
   * @return status
   *   SAVED_UPDATED | SAVED_NEW | SAVED_DELETED
   */
  protected function update_string($i18nstring, $options = array()) {
    if (!empty($i18nstring->source)) {
      $status = $this->add_string($i18nstring, $options);
    }
    else {
      $status = $this->remove_string($i18nstring, $options);
    }
    return $status;
  }
  /**
   * Update string translation.
   */
  function update_translation($context, $langcode, $translation) {
    if ($source = $this->get_source($context, $translation)) {
      db_insert('locales_target')
        ->fields(array(
          'lid' => $source->lid,
          'language' => $langcode,
          'translation' => $translation,
        ))
        ->execute();
    }
  }
}

/**
 * Menu callback. Saves a string translation coming as POST data.
 */
function i18n_string_l10n_client_save_string() {
  global $user, $language;

  if (user_access('use on-page translation')) {
    $textgroup = !empty($_POST['textgroup']) ? $_POST['textgroup'] : 'default';
    // Default textgroup will be handled by l10n_client module
    if ($textgroup == 'default') {
      l10n_client_save_string();
    }
    elseif (isset($_POST['source']) && isset($_POST['target']) && !empty($_POST['form_token']) && drupal_valid_token($_POST['form_token'], 'l10n_client_form')) {
      $translation = new Stdclass();
      $translation->language = $language->language;
      $translation->source = $_POST['source'];
      $translation->translation = $_POST['target'];
      $translation->textgroup = $textgroup;
      i18n_string_save_translation($translation);
    }
  }
}

/**
 * Import translation for a given textgroup.
 *
 * @TODO Check string format properly
 *
 * This will update multiple strings if there are duplicated ones
 *
 * @param $langcode
 *   Language code to import string into.
 * @param $source
 *   Source string.
 * @param $translation
 *   Translation to language specified in $langcode.
 * @param $plid
 *   Optional plural ID to use.
 * @param $plural
 *   Optional plural value to use.
 * @return
 *   The number of strings updated
 */
function i18n_string_save_translation($context) {
  include_once 'includes/locale.inc';

  $query = db_select('locales_source', 's')
    ->fields('s', array('lid'))
    ->fields('i', array('format'))
    ->condition('s.source', $context->source)
    ->condition('s.textgroup', $context->textgroup);
  $query->leftJoin('i18n_strings', 'i', 's.lid = i.lid');
  $result->execute()->fetchAll(PDO::FETCH_OBJ);

  $count = 0;
  foreach ($result as $source) {
    // If we have a format, check format access. Otherwise do regular check.
    if ($source->format ? filter_access($source->format) : locale_string_is_safe($translation)) {
      $exists = (bool) db_select('locales_target', 'l')
        ->fields('l', array('lid'))
        ->condition('lid', $source->lid)
        ->condition('language', $langcode)
        ->execute()
        ->fetchColumn();
      if (!$exists) {
        // No translation in this language.
        db_insert('locales_target')
          ->fields(array(
            'lid' => $source->lid,
            'language' => $langcode,
            'translation' => $translation,
          ))
          ->execute();
      }
      else {
        // Translation exists, overwrite
        db_update('locales_target')
          ->fields(array('translation' => $translation))
          ->condition('language', $langcode)
          ->condition('lid', $source->lid)
          ->execute();
      }
      $count ++;
    }
  }
  return $count;
}

/**
 * Update context for strings.
 *
 * As some string locations depend on configurable values, the field needs sometimes to be updated
 * without losing existing translations. I.e:
 * - profile fields indexed by field name.
 * - content types indexted by low level content type name.
 *
 * Example:
 *  'profile:field:oldfield:*' -> 'profile:field:newfield:*'
 */
function i18n_string_update_context($oldname, $newname) {
  // Get context replacing '*' with empty string.
  $oldcontext = i18n_string_context(str_replace('*', '', $oldname));
  $newcontext = i18n_string_context(str_replace('*', '', $newname));

  // Get location with placeholders.
  $location = i18n_string_location(str_replace('*', '%', $oldname));
  foreach (array('textgroup', 'type', 'objectid', 'property') as $field) {
    if ((!empty($oldcontext->$field) || !empty($newcontext->$field)) && $oldcontext->$field != $newcontext->$field) {
      $replace[$field] = $newcontext->$field;
    }
  }

  // Query and replace if there are any fields. It is possible that under some circumstances fields are the same
  if (!empty($replace)) {
    $query = db_select('locales_source', 's')
      ->fields('s')
      ->fields('i', array('type', 'objectid', 'property'))
      ->condition('s.textgroup', $oldcontext->textgroup)
      ->condition('s.location', $location, 'LIKE');
    $query->leftJoin('i18n_strings', 'i', 's.lid = i.lid');
    $result = $query->execute()->fetchAll(PDO::FETCH_OBJ);

    foreach ($result as $source) {
      // Make sure we have string and context.
      $context = i18n_string_context($oldcontext->textgroup .':'. $source->location);
      foreach ($replace as $field => $value) {
        $context->$field = $value;
      }

      // Update source string.
      db_update('locales_source')
        ->fields(array(
          'textgroup' => $context->textgroup,
          'location' => i18n_string_location($context),
        ))
        ->condition('lid', $source->lid)
        ->execute();

      // Update object data.
      db_update('i18n_strings')
      ->fields(array(
        'type' => $context->type,
        'objectid' => $context->objectid,
        'property' => $context->property,
      ))
      ->condition('lid', $source->lid)
      ->execute();
    }
    drupal_set_message(t('Updating string names from %oldname to %newname.', array('%oldname' => $oldname, '%newname' => $newname)));
  }
}
