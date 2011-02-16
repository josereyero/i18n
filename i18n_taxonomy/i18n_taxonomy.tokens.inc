<?php
// $Id: taxonomy.tokens.inc,v 1.10 2010/11/14 00:25:44 dries Exp $

/**
 * @file
 * Builds placeholder replacement tokens for taxonomy terms and vocabularies.
 */

/**
 * Implements hook_token_info().
 */
function i18n_taxonomy_token_info() {

  // Taxonomy term related variables.
  $term['i18n-name'] = array(
    'name' => t("Name (localized)"),
    'description' => t("The name of the taxonomy term."),
  );
  $term['i18n-description'] = array(
    'name' => t("Description (localized)"),
    'description' => t("The optional description of the taxonomy term."),
  );

  // Taxonomy vocabulary related variables.
  $vocabulary['i18n-name'] = array(
    'name' => t("Name (localized)"),
    'description' => t("The name of the taxonomy vocabulary."),
  );
  $vocabulary['i18n-description'] = array(
    'name' => t("Description (localized)"),
    'description' => t("The optional description of the taxonomy vocabulary."),
  );

  // Chained tokens for taxonomies
  $term['i18n-vocabulary'] = array(
    'name' => t("Vocabulary (localized)"),
    'description' => t("The vocabulary the taxonomy term belongs to."),
    'type' => 'vocabulary',
  );
  $term['i18n-parent'] = array(
    'name' => t("Parent term (localized)"),
    'description' => t("The parent term of the taxonomy term, if one exists."),
    'type' => 'term',
  );

  return array(
    'tokens' => array(
      'term' => $term,
      'vocabulary' => $vocabulary,
    ),
  );
}

/**
 * Implements hook_tokens().
 */
function i18n_taxonomy_tokens($type, $tokens, array $data = array(), array $options = array()) {
  $replacements = array();
  $sanitize = !empty($options['sanitize']);

  if ($type == 'term' && !empty($data['term'])) {
    $term = $data['term'];

    foreach ($tokens as $name => $original) {
      switch ($name) {

        case 'i18n-name':
          $replacements[$original] = $sanitize ? check_plain($term->name) : $term->name;
          break;

        case 'i18n-description':
          $replacements[$original] = $sanitize ? check_markup($term->description, $term->format, '', TRUE) : $term->description;
          break;

        case 'i18n-vocabulary':
          $vocabulary = taxonomy_vocabulary_load($term->vid);
          $replacements[$original] = check_plain(i18n_taxonomy_vocabulary_name($vocabulary));
          break;

        case 'i18n-parent':
          if ($parents = taxonomy_get_parents($term->tid)) {
            $parent = array_pop($parents);
            $replacements[$original] = check_plain(i18n_taxonomy_term_name($parent));
          }
          break;
      }
    }
  }

  elseif ($type == 'vocabulary' && !empty($data['vocabulary'])) {
    $vocabulary = $data['vocabulary'];

    foreach ($tokens as $name => $original) {
      switch ($name) {

        case 'i18n-name':
          $name = i18n_taxonomy_vocabulary_name($vocabulary, $langcode);
          $replacements[$original] = $sanitize ? check_plain($name) : $name;
          break;

        case 'i18n-description':
          $description = i18n_object_langcode($vocabulary) ? $vocabulary->description : i18n_string(array('taxonomy', 'vocabulary', $vocabulary->vid, 'description'), $vocabulary->description, array('langcode' => $langcode));
          $replacements[$original] = $sanitize ? filter_xss($description) : $description;
          break;
      }
    }
  }

  return $replacements;
}
