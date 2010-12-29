<?php 
/**
 * @file
 *   API for internationalization strings
 */


/**
 * Refresh all user defined strings for a given text group.
 *
 * @param $group
 *   Text group to refresh
 * @param $delete
 *   Optional, delete existing (but not refresed, strings and translations)
 * @return Boolean
 *   True if the strings have been refreshed successfully. False otherwise.
 */
function i18nstrings_refresh_group($group, $delete = FALSE) {
  // Check for the refresh callback
  $refresh_callback = i18nstrings_group_info($group, 'refresh callback');
  if (!$refresh_callback) {
    return FALSE;
  }
  // Delete data from i18n_strings so it is recreated
  db_query("DELETE FROM {i18n_strings} WHERE lid IN (SELECT lid FROM {locales_source} WHERE textgroup = :textgroup)", array(':textgroup' => $group));

  $result = call_user_func($refresh_callback);

  // Now delete all source strings that were not refreshed
  if ($result && $delete) {
    $query = db_select('locales_source', 's')
      ->fields('s')
      ->condition('s.textgroup', $group)
      ->condition('i.lid', NULL);
    $query->leftJoin('i18n_strings', 'i', 's.lid = i.lid');
    $result = $query->execute()->fetchAll(PDO::FETCH_OBJ);
    foreach ($result as $source) {
      db_delete('locales_target')->condition('lid', $source->lic)->execute();
      db_delete('locales_source')->condition('lid', $source->lic)->execute();
    }
  }

  cache_clear_all('locale:'. $group .':', 'cache', TRUE);
  return $result;
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
