<?php
// $Id: i18n_path.admin.inc,v 1.1.2.1 2011/02/14 11:36:29 jareyero Exp $
/**
 * @file
 * Administration pages for path translation.
 */

/**
 * Path overview page
 */
function i18n_path_admin_overview() {
  $default = language_default('language');
  $result = db_select('i18n_path', 'p')->fields('p')->execute();
  foreach ($result as $path) {
    $paths[$path->tpid][$path->language] = $path->path;
  }
  if (!empty($paths)) {
    $build['paths'] = array(
      '#theme' => 'table',
      '#header' => array(t('Paths'), t('Operations')),
    );
    foreach ($paths as $tpid => $set) {
      $items = array();
      foreach ($set as $lang => $path) {
        // We'll see the path alias if any on the link
        $items[] = l($path, $path, array('language' => i18n_language($lang)));
      }
      $build['paths']['#rows'][] = array(
        theme('item_list', array('items' => $items)),
        l(t('Edit'), 'admin/config/regional/i18n/path/edit/' . $tpid),
      );
    }
  }
  else {
    $build['message']['#markup'] = t('No path translations.');
  }
  return $build;
}

/**
 * Path add/edit form
 */
function i18n_path_admin_form($form, $form_state, $tpid = NULL) {
  if ($tpid && is_numeric($tpid)) {
    $paths = db_query('SELECT language, path FROM {i18n_path} WHERE tpid = :tpid', array(':tpid' => $tpid))->fetchAllKeyed();
    $form['tpid'] = array('#type' => 'value', '#value' => $paths ? $tpid : 0);
  }
  else {
    $paths = array();
  }
  $form['paths'] = array('#tree' => TRUE);
  foreach (i18n_language_list() as $langcode => $name) {
    $form['paths'][$langcode] = array(
      '#type' => 'textfield',
      '#title' => check_plain($name),
      '#default_value' => !empty($paths[$langcode]) ? $paths[$langcode] : '',
    );
  }
  $form['controls']['update'] = array('#type' => 'submit', '#value' => t('Save'));
  if ($paths) {
    $form['controls']['delete'] = array('#type' => 'submit', '#value' => t('Delete'));
  }
  return $form;
}

/**
 * Process form submission
 */
function i18n_path_admin_form_submit($form, &$form_state) {
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : NULL;
  $tpid = !empty($form_state['values']['tpid']) ? $form_state['values']['tpid'] : 0;

  if ($op == t('Save') && ($paths = array_filter($form_state['values']['paths']))) {
    if (i18n_path_save_translations($paths, $tpid)) {
      drupal_set_message(t('The path translation has been saved.'));
    }
    else {
      drupal_set_message(t('Cannot save the path translation. Please try again later.'), 'error');
    }
  }
  elseif ($op == t('Delete')) {
    db_delete('i18n_path')->condition('tpid', $tpid)->execute();
    drupal_set_message(t('The path translation has been deleted.'));
  }
  $form_state['redirect'] = 'admin/config/regional/i18n/path';
}

/**
 * Save path translation set
 */
function i18n_path_save_translations($paths, $tpid = NULL) {
  $paths = array_filter($paths);
  if (lock_acquire('i18n_path')) {
    if ($tpid) {
      db_delete('i18n_path')->condition('tpid', $tpid)->execute();
    }
    else {
      $tpid = 1 + (int)db_query('SELECT MAX(tpid) FROM {i18n_path}')->fetchField();
    }
    foreach ($paths as $langcode => $path) {
      db_insert('i18n_path')
        ->fields(array('tpid' => $tpid, 'language' => $langcode, 'path' => $path))
        ->execute();
    }
    lock_release('i18n_path');
    return $tpid;
  }  
}