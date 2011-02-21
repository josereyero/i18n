<?php
// $Id: i18n_path.admin.inc,v 1.1.2.2 2011/02/19 17:50:42 jareyero Exp $
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
    $paths[$path->tsid][$path->language] = $path->path;
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
        l(t('Edit'), 'admin/config/regional/i18n_translation/path/edit/' . $tpid),
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
function i18n_path_admin_form($form, $form_state, $translation_set = NULL) {
  $form['translation_set'] = array('#type' => 'value', '#value' => $translation_set);
  if ($translation_set) {
    $paths = $translation_set->get_translations();
  }
  else {
    $paths = array();
  }
  $form['title'] = array(
    '#title' => t('Title'),
    '#type' => 'textfield',
    '#default_value' => $translation_set ? $translation_set->title : '',
    '#description' => t('Optional descriptive name for this set.'),
  );
  $form['translations'] = array(
    '#type' => 'fieldset',
    '#title' => t('Translations'),
    '#tree' => TRUE,
    '#description' => t('Enter system paths that will be considered as translations of each other.'),
  );
  foreach (i18n_language_list() as $langcode => $name) {
    $form['translations'][$langcode] = array(
      '#type' => 'textfield',
      '#title' => check_plain($name),
      '#default_value' => !empty($paths[$langcode]) ? $paths[$langcode]->path : '',
    );
  }
  $form['controls']['update'] = array('#type' => 'submit', '#value' => t('Save'));
  if ($translation_set) {
    $form['controls']['delete'] = array('#type' => 'submit', '#value' => t('Delete'));
  }
  return $form;
}

/**
 * Process form validation
 */
function i18n_path_admin_form_validate($form, &$form_state)  {
  if ($form_state['values']['op'] == t('Save') && !array_filter($form_state['values']['translations'])) {
    form_set_error('paths', t('There are no path translations to save.'));
  }
}

/**
 * Process form submission
 */
function i18n_path_admin_form_submit($form, &$form_state) {
  $translation_set = $form_state['values']['translation_set'];

  switch ($form_state['values']['op']) {
    case t('Save'):
      $paths = array_filter($form_state['values']['translations']);
      $translation_set = $translation_set ? $translation_set : i18n_translation_set_create('path');
      $translations = $translation_set->get_translations();
      $translation_set->translations = array();
      foreach ($paths as $lang => $path) {
        if (isset($translations[$lang])) {
          $translations[$lang]->path = $path;
          $translation_set->translations[$lang]->path = $path;
        }
        else {
          $translation_set->translations[$lang] = $path;
        }
      }
      $translation_set->save(TRUE);
      drupal_set_message(t('The path translation has been saved.'));
      break;
    case t('Delete'):
      $translation_set->delete(TRUE);
      drupal_set_message(t('The path translation has been deleted.'));
      break;
  }
  $form_state['redirect'] = 'admin/config/regional/i18n_translation/path';
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