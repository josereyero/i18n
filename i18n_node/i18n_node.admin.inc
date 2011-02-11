<?php
// $Id: i18n.admin.inc,v 1.2.2.7 2009/01/21 13:08:35 jareyero Exp $

/**
 * @file
 * Extended multilanguage administration and module settings UI.
 */

/**
 * Form builder function.
 */
function i18n_node_admin_settings() {
  // Node translation links setting.
  $form['links'] = array(
    '#type' => 'fieldset',
    '#title' => t('Content translation links'),
  );
  $form['links']['i18n_hide_translation_links'] = array(
    '#type' => 'checkbox',
    '#title' => t('Hide content translation links'),
    '#description' => t('Hide the links to translations in content body and teasers. If you choose this option, switching language will only be available from the language switcher block.'),
    '#default_value' => variable_get('i18n_hide_translation_links', 0),
  );
  $form['links']['i18n_translation_switch'] = array(
    '#type' => 'checkbox',
    '#title' => t('Switch interface for translating'),
    '#default_value' => variable_get('i18n_translation_switch', 0),
    '#description' => t('Switch interface language to fit node language when creating or editing a translation. If not checked the interface language will be independent from node language.'),
  );
  return system_settings_form($form);
}