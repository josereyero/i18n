<?php 
/**
 * @file
 *   API for internationalization strings
 */

/**
 * Class wrapper for i18nstrings API
 */
class i18nstrings {
  /**
   * Add source string to the locale tables for translation.
   *
   * It will also add data into i18n_strings table for faster retrieval and indexing of groups of strings.
   * Some string context doesn't have a numeric oid (I.e. content types), it will be set to zero.
   *
   * This function checks for already existing string without context for this textgroup and updates it accordingly.
   * It is intended for backwards compatibility, using already created strings.
   *
   * @param $name
   *   Textgroup and location glued with ':'
   * @param $string
   *   Source string (string in default language)
   * @param $format
   *   Input format, for strings that will go through some filter
   * @return
   *   Update status.
   */
  protected static function add_string($context, $options = array()) {
    $options += array('watchdog' => TRUE);
    // Default return status if nothing happens
    $status = -1;
    // The string may not be allowed for translation depending on its format.
    if (!i18nstrings_check_string($context, $options)) {
      // The format may have changed and it's not allowed now, delete the source string
      return self::remove_string($context, $options);
    }
    elseif ($source = self::get_source($context)) {
      $context->lid = $source->lid;
      if ($source->source != $context->source || $source->location != $context->location) {
        // String has changed, mark translations for update
        $status = self::save_source($source);
        db_update('locales_target')
          ->fields(array('i18n_status' => I18NSTRINGS_STATUS_UPDATE))
          ->condition('lid', $source->lid)
          ->execute();
      }
    }
    else {
      // We have no source string, create it
      $status = self::save_source($context);
    }
    // Update metadata and log to watchdog
    self::save_context($context, $source);
  
    if ($options['watchdog']) {
      $params = i18nstrings_params($context);
      switch ($status) {
        case SAVED_UPDATED:
          watchdog('i18nstrings', 'Updated string %location for textgroup %textgroup: %string', $params);
          break;
        case SAVED_NEW:
          watchdog('i18nstrings', 'Created string %location for text group %textgroup: %string', $params);
          break;
      }     
    }  
    return $status;
  }
  /**
   * Set string object into cache
   */
  protected static function cache_set($context) {
    $strings = &drupal_static('i18nstrings_cache');
    $context->cache = TRUE;
    $strings[$context->language][$context->textgroup][$context->context] = $context;
  }
  
  /**
   * Get string object from cache
   */
  protected static function cache_get($context) {
    $strings = &drupal_static('i18nstrings_cache');
    if (isset($strings[$context->language][$context->textgroup][$context->context])) {
      return $strings[$context->language][$context->textgroup][$context->context];
    }
    else {
      return NULL;
    }
  }  
  /**
   * Check if string is ok for translation
   */
  protected static function check_string($i18nstring, $options = array()) {
    $options += array('messages' => FALSE, 'watchdog' => TRUE);
    if (isset($i18nstring->format) && !i18nstrings_allowed_format($i18nstring->format)) {
      // This format is not allowed, so we remove the string, in this case we produce a warning
      drupal_set_message(t('The string %location for textgroup %textgroup is not allowed for translation because of its input format.', i18nstrings_params($i18nstring)), 'warning');
      return FALSE;
    }
    else {
      return TRUE;
    }
  }
  /**
   * Get source string provided a string context.
   *
   * @param $context
   *   Context string or object.
   * @return
   *   Context object if it exists.
   */
  protected static function get_source($context) {
    // Search the database using lid if we've got it or textgroup, context otherwise
    $query = db_select('locales_source', 's')->fields('s');
    if (!empty($context->lid)) {
      $query->condition('s.lid', $context->lid);
    }
    else {
      $query->condition('s.textgroup', $context->textgroup);
      $query->condition('s.context', $context->context);
    }
  }
  /**
   * Get translation from the database. Full object with input format.
   *
   * This one doesn't return anything if we don't have the full i18n strings data there
   * to prevent missing data resulting in missing input formats
   */
  protected static function get_translation($context) {
    // First try the cache
    if ($translation = self::cache_get($context)) {
      return $translation->translation === FALSE ? NULL : $translation;
    }
    $query = self::query_string($context);
    $query->leftJoin('locales_target', 't', 's.lid = t.lid');
    $query->fields('t', array('translation', 'i18n_status'));
    $query->condition('t.language', $context->language);    
    // Speed up the query, we just need a row
    $query->range(0, 1);
  
    if ($translation = $query->execute()->fetchObject()) {
      $translation->language = $context->language;
      self::cache_set($translation);
      return $translation;
    }
    else {
      // Set on the cache so we don't search again
      $context->translation = FALSE;
      self::cache_set($context);
      return NULL;
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
      $query->condition('s.context', $context->context);
    }
    return $query;  
  }
  
  /**
   * Remove string object.
   */
  public static function remove_string($i18nstring, $options = array()) {
    $options += array('watchdog' => TRUE);
    if ($source = self::get_source($i18nstring)) {
      db_delete('locales_target')->condition('lid', $source->lid)->execute();
      db_delete('i18n_strings')->condition('lid', $source->lid)->execute();
      db_delete('locales_source')->condition('lid', $source->lid)->execute();
      if ($options['watchdog']) {
        watchdog('i18nstrings', 'Deleted string %location for textgroup %textgroup: %string', i18nstrings_params($i18nstring));
      } 
      return SAVED_DELETED;
    }
  }
  /**
   * Save / update context metadata.
   *
   * There seems to be a race condition sometimes so skip errors, #277711
   * 
   * @param $context
   *   Full string object to be saved
   * @param $source
   *   Source string object
   */
  protected static function save_context($context, $source = NULL) {
    $source = $source ? $source : self::get_source($context);
    if ($source) {
      $context->lid = $source->lid;
      drupal_write_record('i18n_strings', $context, 'lid');
      if ($context->source !== $source->source) {
        // Mark translations as need update
        db_update('locales_target')
          ->condition('lid', $context->lid)
          ->fields(array('i18n_status' => I18NSTRINGS_STATUS_UPDATE))
          ->execute();
      }
      return SAVED_UPDATED;
    }
    else {
      self::save_source($context);
      drupal_write_record('i18n_strings', $context);
      return SAVED_NEW;
    }
  }

  /**
   * Save source string (create / update)
   */
  protected static function save_source($source) {
    if (!isset($source->version)) {
      $source->version = 1;
    }
    return drupal_write_record('locales_source', $source, !empty($source->lid) ? 'lid' : array());
  }


  /**
   * Translate string object
   * 
   * @param $i18nstring
   *   String object
   * @param $options
   *   Array with aditional options
   */
  public static function translate_string($i18nstring, $options = array()) {
    $i18nstring->language = isset($options['langcode']) ? $options['langcode'] : i18nstrings_language()->language;
    // Search for existing translation (result will be cached in this function call)
    if ($translation = self::get_translation($i18nstring)) {
      return $translation;
    }
    else {
      return $i18nstring;
    }
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
  public static function update_string($i18nstring, $options = array()) {
    if (!empty($i18nstring->source)) {
      $status = self::add_string($i18nstring, $options);
    }
    else {
      $status = self::remove_string($i18nstring, $options);
    }
    return $status;
  }
  
}

/**
 * Get refresh callback for a text group.
 *
 * @param $group
 * @param $reset
 *   Whether to reset the internal cache for the textgroup.
 *
 * @return callback
 */
function i18nstrings_group_info($group = NULL, $property = NULL, $reset = FALSE) {
  $info = &drupal_static(__FUNCTION__ , NULL, $reset);

  if (!isset($info)) {
    $info = module_invoke_all('locale', 'info');
  }

  if ($group && $property) {
    return isset($info[$group][$property]) ? $info[$group][$property] : NULL;
  }
  elseif ($group) {
    return isset($info[$group]) ? $info[$group] : array();
  }
  else {
    return $info;
  }
}

/**
 * Menu callback. Saves a string translation coming as POST data.
 */
function i18nstrings_l10n_client_save_string() {
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
      i18nstrings_save_translation($translation);
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
function i18nstrings_save_translation($context) {
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
 * Provides interface translation services.
 *
 * This function is called from i18nstrings() to translate a string if needed.
 *
 * @param $textgroup
 *
 * @param $string
 *   A string to look up translation for. If omitted, all the
 *   cached strings will be returned in all languages already
 *   used on the page.
 * @param $langcode
 *   Language code to use for the lookup.
 * @param $reset
 *   Whether to reset the internal cache for the textgroup.
 */
function i18nstrings_textgroup($textgroup, $string = NULL, $langcode = NULL, $reset = FALSE) {
  global $language;
  $locale_t = &drupal_static(__FUNCTION__ , NULL, $reset);

  // Return all cached strings if no string was specified.
  if (!isset($string)) {
    return isset($locale_t[$textgroup]) ? $locale_t[$textgroup] : array();
  }

  $langcode = isset($langcode) ? $langcode : $language->language;

  // Store database cached translations in a static variable.
  if (!isset($locale_t[$langcode])) {
    $locale_t[$langcode] = array();
    // Disabling the usage of string caching allows a module to watch for
    // the exact list of strings used on a page. From a performance
    // perspective that is a really bad idea, so we have no user
    // interface for this. Be careful when turning this option off!
    if (variable_get('locale_cache_strings', 1) == 1) {
      if ($cache = cache_get('locale:'. $textgroup .':'. $langcode, 'cache')) {
        $locale_t[$textgroup][$langcode] = $cache->data;
      }
      else {
        // Refresh database stored cache of translations for given language.
        // We only store short strings used in current version, to improve
        // performance and consume less memory.
        $query = db_select('locales_source', 's')
          ->fields('s', array('source'))
          ->fields('t', array('translation', 'language'))
          ->condition('s.textgroup', $textgroup)
          ->condition('s.version', VERSION)
          ->where('LENGTH(s.source) < 75');
        $query->leftJoin('locales_target', 't', 's.lid = t.lid AND t.language = :language', array(':language' => $langcode));
        $result = $query->execute()->fetchAll(PDO::FETCH_OBJ);
        foreach ($result as $data) {
          $locale_t[$textgroup][$langcode][$data->source] = (empty($data->translation) ? TRUE : $data->translation);
        }
        cache_set('locale:'. $textgroup .':'. $langcode, $locale_t[$textgroup][$langcode]);
      }
    }
  }

  // If we have the translation cached, skip checking the database
  if (!isset($locale_t[$textgroup][$langcode][$string])) {

    // We do not have this translation cached, so get it from the DB.
    $query= db_select('locales_source', 's')
      ->fields('s', array('lid', 'version'))
      ->fields('t', array('translation'))
      ->condition('s.source', $string)
      ->condition('s.textgroup', $textgroup);
    $query->leftJoin('locales_target', 't', 's.lid = t.lid AND t.language = :language', array(':language' => $langcode));
    $translation = $query->execute()->fetchObject();
    if ($translation) {
      // We have the source string at least.
      // Cache translation string or TRUE if no translation exists.
      $locale_t[$textgroup][$langcode][$string] = (empty($translation->translation) ? TRUE : $translation->translation);

      if ($translation->version != VERSION) {
        // This is the first use of this string under current Drupal version. Save version
        // and clear cache, to include the string into caching next time. Saved version is
        // also a string-history information for later pruning of the tables.
        db_query_range(
          "UPDATE {locales_source} SET version = :version WHERE lid = :lid",
          0, 1,
          array(':version' => VERSION, ':lid' => $translation->lid)
        );
        cache_clear_all('locale:'. $textgroup .':', 'cache', TRUE);
      }
    }
    else {
      // We don't have the source string, cache this as untranslated.
      db_insert('locales_source')
        ->fields(array(
          'location' => request_uri(),
          'source' => $string,
          'textgroup' => $textgroup,
          'version' => VERSION,
        ))
        ->execute();
      $locale_t[$langcode][$string] = TRUE;
      // Clear locale cache so this string can be added in a later request.
      cache_clear_all('locale:'. $textgroup .':', 'cache', TRUE);
    }
  }

  return ($locale_t[$textgroup][$langcode][$string] === TRUE ? $string : $locale_t[$textgroup][$langcode][$string]);
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
function i18nstrings_update_context($oldname, $newname) {
  // Get context replacing '*' with empty string.
  $oldcontext = i18nstrings_context(str_replace('*', '', $oldname));
  $newcontext = i18nstrings_context(str_replace('*', '', $newname));

  // Get location with placeholders.
  $location = i18nstrings_location(str_replace('*', '%', $oldname));
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
      $context = i18nstrings_context($oldcontext->textgroup .':'. $source->location);
      foreach ($replace as $field => $value) {
        $context->$field = $value;
      }

      // Update source string.
      db_update('locales_source')
        ->fields(array(
          'textgroup' => $context->textgroup,
          'location' => i18nstrings_location($context),
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
