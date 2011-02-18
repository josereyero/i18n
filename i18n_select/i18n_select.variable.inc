<?php
/**
 * @file
 * Variable information
 */

/**
 * Implements hook_variable_info()
 */
function i18n_select_variable_info($options = array()) {
  $variables['i18n_select_missing_translation'] = array(
    'title' => t('When translation is missing, show content in the default language', array(), $options),
    'type' => 'boolean',
    'default' => FALSE,
    'group' => 'i18n',
  );
  $variables['i18n_select_nodes'] = array(
    'title' => t('Select nodes by language', array(), $options),
    'type' => 'boolean',
    'default' => FALSE,
    'group' => 'i18n',
  );
  $variables['i18n_select_nodes'] = array(
    'title' => t('Select taxonomy terms by language', array(), $options),
    'type' => 'boolean',
    'default' => FALSE,
    'group' => 'i18n',
  );
  return $variables;
}