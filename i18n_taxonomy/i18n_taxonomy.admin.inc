<?php
// $Id: i18n_taxonomy.admin.inc,v 1.1.2.3 2011/02/14 11:36:29 jareyero Exp $

/**
 * @file
 * Helper functions for taxonomy administration.
 */

/**
 * This is the callback for taxonomy translations.
 *
 * Gets the urls:
 *  admin/content/taxonomy/%taxonomy_vocabulary/translation
 *  admin/content/taxonomy/i18n/term/xx
 *  admin/content/taxonomy/i18n/term/new/xx
 *  admin/content/taxonomy/vid/translation/op/trid
 */
function i18n_taxonomy_page_vocabulary($vocabulary, $op = NULL, $tid = NULL) {
  switch ($op) {
    case 'edit':
      drupal_set_title(t('Edit term translations'));
      $output = drupal_get_form('i18n_taxonomy_translation_term_form', $vocabulary, $tid);
      break;

    default:
      $output = i18n_taxonomy_translation_overview($vocabulary);
  }
  return $output;
}

/**
 * Produces a vocabulary translation form.
 */
function i18n_taxonomy_translation_term_form($form, $form_state, $vocabulary, $translation_set = NULL) {
  if ($translation_set) {
    $trid = $translation_set['trid'];
    $translations = $translation_set['translations'];
  }
  else {
    $trid = 0;
    $translations = array();
  }
  $form['vocabulary'] = array('#type' => 'value', '#value' => $vocabulary);
  $form['trid'] = array(
    '#type' => 'hidden',
    '#value' => $trid
  );
  $form['translations'] = array('#tree' => TRUE);
  // List of terms for languages.
  foreach (i18n_language_list() as $lang => $langname) {
    $form['translations'][$lang] = array(
      '#title' => $langname,
      '#type' => 'textfield',
      '#default_value' => isset($translations[$lang]) ? $translations[$lang]->name : '',
      '#autocomplete_path' => 'i18n/taxonomy/autocomplete/' . $vocabulary->machine_name . '/' . $lang,
      '#langcode' => $lang,
      '#size' => 80,
      '#maxlength' => 1024,
      '#element_validate' => array('i18n_taxonomy_autocomplete_validate'),      
    );

  }
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save')
  );
  if ($trid) {
    $form['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete')
    );
  }
  $form['destination'] = array(
    '#type' => 'hidden',
    '#value' => 'admin/content/taxonomy/'. arg(3) .'/translation'
  );
  return $form;
}

/**
 * Form element validate handler for taxonomy term autocomplete element.
 */
function i18n_taxonomy_autocomplete_validate($element, &$form_state) {
  // Autocomplete widgets do not send their tids in the form, so we must detect
  // them here and process them independently.
  $value = array();
  if ($tags = $element['#value']) {
    // Collect candidate vocabularies.
    $vocabulary = $form_state['values']['vocabulary'];
    $vocabularies[$vocabulary->vid] = $vocabulary;

    // Translate term names into actual terms.
    $typed_terms = drupal_explode_tags($tags);
    foreach ($typed_terms as $typed_term) {
      // See if the term exists in the chosen vocabulary and return the tid;
      // otherwise, create a new 'autocreate' term for insert/update.
      if ($possibilities = taxonomy_term_load_multiple(array(), array('name' => trim($typed_term), 'vid' => $vocabulary->vid, 'language' => $element['#langcode']))) {
        $term = array_pop($possibilities);
      }
      else {
        $vocabulary = reset($vocabularies);
        $term = array(
          'tid' => 'autocreate',
          'vid' => $vocabulary->vid,
          'name' => $typed_term,
          'vocabulary_machine_name' => $vocabulary->machine_name,
          'language' => $element['#langcode'],
        );
      }
      $value[] = (array)$term;
    }
  }

  form_set_value($element, $value, $form_state);
}

/**
 * Form callback: Process vocabulary translation form.
 */
function i18n_taxonomy_translation_term_form_submit($form, &$form_state) {
  switch ($form_state['values']['op']) {
    case t('Save'):
      if (i18n_taxonomy_translation_save($form_state['values']['translations'], $form_state['values']['trid'])) {
        drupal_set_message(t('Term translations have been updated.'));
      }
      else {
        drupal_set_message(t('Cannot update term translations'), 'error');
      }
      break;

    case t('Delete'):
      // Delete old translations for this trid.
      db_update('taxonomy_term_data')
        ->fields(array('trid' => 0))
        ->condition('trid', $form_state['values']['trid'])
        ->execute();
      drupal_set_message(t('The term translation has been deleted.'));
      $form_state['redirect'] = 'admin/structure/taxonomy/' . $form_state['values']['vocabulary']->machine_name . '/translation';
      break;
  }
}

/**
 * Save taxonomy term translations.
 *
 * @param $terms
 *   Array of terms indexed by language.
 * @param $trid
 *   Optional translation set id.
 */
function i18n_taxonomy_translation_save($terms, $trid = 0) {
  // Now set a translation set with all these terms. We need some table locking to avoid race conditions.
  // when other translations created simulaneously. @TODO Find a better way.
  if (lock_acquire('i18n_taxonomy')) {
    // Delete old translations for this trid.
    if ($trid) {
      db_update('taxonomy_term_data')
        ->fields(array('trid' => 0))
        ->condition('trid', $trid)
        ->execute();
    }
    // Now pick up all the tids in an array.
    $translations = array();
    foreach (i18n_language_list() as $lang => $name) {
      if (!empty($terms[$lang]) && ($item = reset($terms[$lang]))) {
        if ($item['tid'] == 'autocreate') {
          $term = (object) $item;
          unset($term->tid);
          taxonomy_term_save($term);
        }
        else {
          $term = (object) $item;
        }
        $translations[$lang] = $term->tid;
      }
    }

    if (count($translations)) {
      $trid = (is_numeric($trid) && $trid) ? $trid : i18n_taxonomy_next_trid();
      $params = array_merge(array($trid), $translations);
      db_update('taxonomy_term_data')
        ->fields(array('trid' => $trid))
        ->condition('tid', $translations)
        ->execute();
    }
    lock_release('i18n_taxonomy');
    return $trid;
  }
  else {
    return FALSE;
  }
}

/**
 * Get next free trid.
 */
function i18n_taxonomy_next_trid() {
  return 1 + (int)db_query('SELECT MAX(trid) FROM {taxonomy_term_data}')->fetchField();
}

/**
 * Generate a tabular listing of translations for vocabularies.
 */
function i18n_taxonomy_translation_overview($vocabulary) {
  drupal_set_title(check_plain($vocabulary->name));
  $output = '';

  $languages = i18n_language_list();
  $header = array_merge($languages, array(t('Operations')));
  $links = array();
  $types = array();
  // Get terms/translations for this vocab.
  $result = db_query('SELECT * FROM {taxonomy_term_data} t WHERE vid = :vid', array(':vid' => $vocabulary->vid));
  $terms = $messages = array();
  foreach($result as $term) {
    if ($term->trid && $term->language) {
      $terms[$term->trid][$term->language] = $term;
    }
  }
  // Reorder data for rows and languages.
  $rows = array();
  foreach ($terms as $trid => $terms) {
    $thisrow = array();
    foreach ($languages as $lang => $name) {
      if (array_key_exists($lang, $terms)) {
        $thisrow[] = l($terms[$lang]->name, 'taxonomy/term/'. $terms[$lang]->tid);
      }
      else {
        $thisrow[] = '--';
      }
    }
    $thisrow[] = l(t('edit'), "admin/structure/taxonomy/$vocabulary->machine_name/translation/edit/$trid");
    $rows[] = $thisrow;
  }
  if ($rows) {
    $build['translations'] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows
    );
  }
  else {
    $build['message']['#markup'] = t('No translations defined for this vocabulary.');
  }
  return $build;
}
