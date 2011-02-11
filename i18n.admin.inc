<?php
// $Id: i18n.admin.inc,v 1.2.2.7 2009/01/21 13:08:35 jareyero Exp $

/**
 * @file
 * Extended multilanguage administration and module settings UI.
 */

/**
 * Form builder function.
 *
 * TO DO: Add exclude paths for content selection
 * Some options have been removed from previous versions:
 * - Languages are now taken from locale module unless defined in settings file.
 * - Language dependent tables are authomatically used if defined in settings file.
 */
function i18n_admin_settings() {
  $form['i18n_language_list'] = array(
    '#type' => 'radios',
    '#title' => t('Languages for content'),
    '#default_value' => variable_get('i18n_language_list', I18N_LANGUAGE_ENABLED),
    '#options' => array(
       I18N_LANGUAGE_ENABLED => t('Enabled languages only.'),
       I18N_LANGUAGE_EXTENDED => t('All defined languages will be allowed.'),
    ),
    '#description' => t('Determines which languages will be allowed for content creation.'),
  );

  return system_settings_form($form);
}


