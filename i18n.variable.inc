<?php
/**
 * @file
 * Variable information
 */

/**
 * Implements hook_variable_group_info()
 */
function i18n_variable_group_info() {
  $groups['i18n'] = array(
    'title' => t('Multilingual settings'),
    'description' => t('Mixed options for multilingual sites.'),
    'access' => 'administer site configuration',
    'path' => 'admin/config/regional/i18n',     
  );
  return $groups;
}

/**
 * Implements hook_variable_info()
 */
function i18n_variable_info($options = array()) {
  $variables['i18n_language_list'] = array(
    'title' => t('Languages for content', array(), $options),
    'description' => t('Determines which languages will be allowed for content creation.', array(), $options),
    'type' => 'select',
    'options callback' => 'i18n_variable_option_list',
    'default' => I18N_LANGUAGE_ENABLED,
    'group' => 'i18n',
  );
  return $variables;
}

/**
 * Options for content languages
 */
function i18n_variable_option_list($variable, $options = array()) {
  return array(
    I18N_LANGUAGE_ENABLED => t('Enabled languages only.', array(), $options),
    I18N_LANGUAGE_EXTENDED => t('All defined languages will be allowed.', array(), $options),
  );
}