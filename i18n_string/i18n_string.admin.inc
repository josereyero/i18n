<?php
// $Id: i18n_string.admin.inc,v 1.1.2.9 2010/03/03 17:39:24 jareyero Exp $

// Load locale library
include_once './includes/locale.inc';

/**
 * Form callback. Refresh textgroups.
 */
function i18n_string_admin_refresh_form() {
  module_load_include('inc', 'i18n_string');
  // Select textgroup/s. Just the ones that have a 'refresh callback'
  $groups = module_invoke_all('locale', 'groups');
  unset($groups['default']);
  foreach (array_keys($groups) as $key) {
    if (!i18n_string_info($key, 'refresh callback')) {
      unset($groups[$key]);
    }
  }
  $form['groups'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Select text groups'),
    '#options' => $groups,
    '#description' => t('If a text group is no showing up here it means this feature is not implemented for it.'),
  );
  $form['refresh'] = array(
    '#type' => 'submit',
    '#value' => t('Refresh strings'),
    '#suffix' => '<p>'. t('This will create all the missing strings for the selected text groups.') .'</p>',
  );
  // Get all languages, except default language.
  $languages = locale_language_list('name', TRUE);
  unset($languages[language_default('language')]);
  $form['languages'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Select languages'),
    '#options' => $languages,
  );
  $form['update'] = array(
    '#type' => 'submit',
    '#value' => t('Update translations'),
    '#suffix' => '<p>'. t('This will fetch all existing translations from the localization tables for the selected text groups and languages.') .'</p>',
  );
  return $form;
}

/**
 * Form submission.
 */
function i18n_string_admin_refresh_form_submit($form, &$form_state) {
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';
  $groups = array_filter($form_state['values']['groups']);
  $languages = array_filter($form_state['values']['languages']);
  $group_names = module_invoke_all('locale', 'groups');
  if ($op == t('Refresh strings') && $groups) {
    foreach ($groups as $group) {
      if (i18n_string_refresh_group($group, TRUE)) {
        drupal_set_message(t("Successfully refreshed strings for %group", array('%group' => $group_names[$group])));
      }
      else {
        drupal_set_message(t("Cannot refresh strings for %group.", array('%group' => $group_names[$group])), 'warning');
      }
    }
  }
  elseif ($op == t('Update translations') && $groups && $languages) {
    $count = 0;
    foreach ($languages as $language) {
      $count += i18n_string_admin_update($language, $groups);
    }
    drupal_set_message(format_plural($count, '1 string has been updated.', '@count strings have been updated.'));
  }
}

/**
 * Update strings for language.
 */
function i18n_string_admin_update($language, $groups) {

  $query = db_select('locales_source', 'g')
    ->fields('g')
    ->fields('t', array('translation'))
    ->fields('i', array('format'))
    ->condition('t2.lid', NULL)
    ->condition('g.textgroup', $groups)
    ->condition('t.language', $language);
  $query->addField('t', 'lid', 'tlid');
  $query->join('locales_source', 's', 'g.source = s.source AND s.lid <> g.lid');
  $query->join('locales_target', 't', 's.lid = t.lid');
  $query->join('locales_target', 't2', 'g.lid = t2.lid');
  $query->join('i18n_strings', 'i', 'i.lid = g.lid');
  $result = $query->execute()->fetchAll(PDO::FETCH_OBJ);

  $count = 0;
  foreach($result as $string) {
    // Just update strings when no input format, otherwise it could be dangerous under some circumstances.
    if (empty($string->format) && !empty($string->translation)) {
      $count++;
      db_insert(locales_target)
      ->fields(array(
        'translation' => $string->translation,
      	'lid' => $string->lid,
      	'language' => $language,
      ))
      ->execute();
    }
  }
  return $count;
}

/**
 * Configure filters for string translation.
 *
 * This has serious security implications so this form needs the 'administer filters' permission
 */
function i18n_string_admin_settings() {
  module_load_include('inc', 'i18n_string');
  include_once './includes/locale.inc';
  // As the user has administer filters permissions we get a full list here
  foreach (filter_formats() as $fid => $format) {
    $format_list[$fid] = $format->name;
  }
  $form['i18n_string_allowed_formats'] = array(
    '#title' => t('Translatable input formats'),
    '#options' => $format_list,
    '#type' => 'checkboxes',
    '#default_value' => variable_get('i18n_string_allowed_formats', array(variable_get('filter_default_format', 1))),
    '#description' => t('Only the strings that have the input formats selected will be allowed by the translation system. All the others will be deleted next time the strings are refreshed.'),
  );
  // Whitelist text groups without formatted strings for backwards compatibility
  $textgroups = module_invoke_all('locale', 'groups');
  unset($textgroups['default']);
  foreach (array_keys($textgroups) as $group) {
    $format = i18n_string_info($group, 'format');
    if (isset($format)) {
      // This one already has format information, so remove from list
      unset($textgroups[$group]);
    }
  }
  // If there are 'old' textgroups, display the bypass option
  if ($textgroups) {
    $form['i18n_string_allowed_textgroups'] = array(
      '#title' => t('Safe text groups'),
      '#options' => $textgroups,
      '#type' => 'checkboxes',
      '#default_value' => variable_get('i18n_string_allowed_textgroups', array()),
      '#description' => t('Select text groups to bypass filter format checking. . <strong>It is unsafe to check this option unless you are sure all the strings from that text groups are safe for translators</strong>. This option is just for backwards compatibility until all the contributed modules implement the new strings API.'),
    );
  }
  elseif (variable_get('i18n_string_allowed_textgroups', 0)) {
    // Just in case there's a leftover variable before we updated some of the modules
    variable_del('i18n_string_allowed_textgroups');
  }
  $form['array_filter'] = array('#type' => 'value', '#value' => TRUE);
  return system_settings_form($form);
}

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
function i18n_string_refresh_group($group, $delete = FALSE) {
  // Check for the refresh callback
  $refresh_callback = i18n_string_info($group, 'refresh callback');
  if (!$refresh_callback) {
    return FALSE;
  }
  // Mark data on locales_source setting version = 0
  db_update('locales_source')
    ->fields(array('version' => 0))
    ->condition('textgroup', $group)
    ->execute();

  $result = call_user_func($refresh_callback);

  // Now delete all source strings that were not refreshed (they don't have a row in i18n_strings)
  if ($result && $delete) {
    $lids = db_select('locales_source', 's')
      ->fields('s', array('lid'))
      ->condition('textgroup', $group)
      ->condition('version', 0)
      ->execute()
      ->fetchCol();

    if ($lids) {
      drupal_set_message(t('Performing cleanup for text group %textgroup, deleting @count left over strings.', array('%textgroup' => $group, '@count' => count($lids))));
      db_delete('locales_target')->condition('lid', $lids)->execute();
      db_delete('locales_source')->condition('lid', $lids)->execute();
      db_delete('i18n_strings')->condition('lid', $lids)->execute();    
    }
  }
  return $result;
}
