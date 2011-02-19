<?php
// $Id: i18n_taxonomy.admin.inc,v 1.1.2.4 2011/02/19 17:50:42 jareyero Exp $

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
  $form['translation_set'] = array('#type' => 'value', '#value' => $translation_set);  
  $translations = $translation_set ? $translation_set->get_translations() : array();
  $form['vocabulary'] = array('#type' => 'value', '#value' => $vocabulary);
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
  if ($translation_set) {
    $form['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete')
    );
  }
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
  $translation_set = $form_state['values']['translation_set'];
  $vocabulary = $form_state['values']['vocabulary'];
  switch ($form_state['values']['op']) {
    case t('Save'):
      $term_translations = array_filter($form_state['values']['translations']);
      foreach ($term_translations as $lang => $lang_terms) {
        $item = reset($lang_terms);
        if ($item['tid'] == 'autocreate') {
          $term = (object) $item;
          unset($term->tid);
          taxonomy_term_save($term);
        }
        else {
          $term = (object) $item;
        }
        $translations[$lang] = $term;    
      }
      if (!empty($translations)) {
        $translation_set = $translation_set ? $translation_set : i18n_translation_set_create('taxonomy_term', $vocabulary->machine_name);
        $translation_set->translations = $translations;
        $translation_set->save(TRUE);
        drupal_set_message(t('Term translations have been updated.'));
      }
      else {
        drupal_set_message(t('There are no translations to be saved.'), 'error');
      }
      break;

    case t('Delete'):
      $translation_set->delete(TRUE);
      drupal_set_message(t('The term translation has been deleted.'));
      $form_state['redirect'] = 'admin/structure/taxonomy/' . $form_state['values']['vocabulary']->machine_name . '/translation';
      break;
  }
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
  $result = db_query('SELECT * FROM {taxonomy_term_data} t WHERE vid = :vid AND i18n_tsid > 0', array(':vid' => $vocabulary->vid));
  $terms = $messages = array();
  foreach($result as $term) {
    if ($term->i18n_tsid && $term->language) {
      $terms[$term->i18n_tsid][$term->language] = $term;
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
