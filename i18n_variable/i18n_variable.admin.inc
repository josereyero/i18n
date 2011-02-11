<?php
// $Id: i18nvariable.module,v 1.1.2.2 2010/09/14 11:30:29 jareyero Exp $
/**
 * @file
 * Administration pages for multilingual variables. Internationalization (i18n) package.
 */

/**
 * Select multilingual variables
 * 
 * There are two lists of variables here. 
 * - i18n_variable_conf, list with high level variable names, some of which are multiple variables
 * - i18n_variable_list, translated list into actual variable names
 */
function i18n_variable_admin_settings($form, $form_state) {
  $translatable = array();
  $current = variable_get('i18n_variable_conf', array());
  foreach (variable_get_info() as $name => $variable) {
    if (!empty($variable['localize'])) {
      $translatable[] = $name;
    }
  }
  // The final list will be the sum of both lists. We may have unknown variables we want to preserve.
  $list = array_unique(array_merge($translatable, $current));
  $list[] = 'unknown_variable';
  $form['variables'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select variables to be translated'),
    '#theme' => 'variable_table_select',
    '#tree' => TRUE,
  );
  foreach ($list as $name) {
    $form['variables'][$name] = array(
      '#type' => 'checkbox',
      '#default_value' => in_array($name, $current),
    );
  }
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save configuration'));
  return $form;
}

/**
 * Handle form submission.
 */
function i18n_variable_admin_settings_submit($form, $form_state) {
  // Get main variable names
  $variables = $form_state['values']['variables'];
  $variables = array_keys(array_filter($variables));
  variable_set('i18n_variable_conf', $variables);
  // Spawn multiple variables and translate into actual variables
  variable_set('i18n_variable_list', variable_children($variables));
  // Clear list from cache
  cache_clear_all('*', I18N_VARIABLE_CACHE, TRUE);
}