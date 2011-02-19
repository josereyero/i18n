<?php
/**
 * @file
 * Variable information
 */

/**
 * Implements hook_variable_info()
 */
function i18n_node_variable_info($options = array()) {
  $variables['i18n_hide_translation_links'] = array(
    'type' => 'boolean',
    'title' => t('Hide content translation links', array(), $options),
    'description' => t('Hide the links to translations in content body and teasers. If you choose this option, switching language will only be available from the language switcher block.', array(), $options),
    'default' => 0,
    'group' => 'i18n',
  );
  $variables['i18n_translation_switch'] = array(
    'type' => 'boolean',
    'title' => t('Switch interface for translating', array(), $options),
    'default' => 0,
    'group' => 'i18n',
    'description' => t('Switch interface language to fit node language when creating or editing a translation. If not checked the interface language will be independent from node language.', array(), $options),
  );
  return $variables;
}