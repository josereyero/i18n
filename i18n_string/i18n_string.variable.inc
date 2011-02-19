<?php
/**
 * @file
 * Variable information
 */

/**
 * Implements hook_variable_info()
 */
function i18n_string_variable_info($options = array()) {
  $variables['i18n_string_translate_langcode_[language]'] = array(
    'type' => 'multiple_language',
    'title' => t('Enable translation for language'),
    'multiple values' => array('type' => 'boolean'),
    'group' => 'i18n',
  );
  $variables['i18n_string_debug'] = array(
    'type' => 'enable',
    'title' => t('Debug string translation', array(), $options),
    'default' => 0,
    'group' => 'debug',
  );
  return $variables;
}