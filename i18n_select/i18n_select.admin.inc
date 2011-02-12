<?php
// $Id: i18n_select.admin.inc,v 1.1.2.1 2010/11/09 10:31:32 brucepearson Exp $

function i18n_select_admin_settings() {
  // Content selection options.
  $form['selection'] = array(
    '#type' => 'fieldset',
    '#title' => t('Language content filtering'),
  );
  
  $form['selection']['mode'] = array(
    '#type' => 'markup',
    '#markup' => '<strong>' . t('Content selection mode') . '</strong>',
  );
  $form['selection']['i18n_select_missing_translation'] = array(
    '#type' => 'checkbox',
    '#title' => t('When translation is missing, show content in the default language'),
    '#default_value' => variable_get('i18n_select_missing_translation', FALSE),
  );

  $form['selection']['types'] = array(
    '#type' => 'markup',
    '#markup' => '<strong>' . t('Content to filter by language') . '</strong>',
  );
  $form['selection']['i18n_select_nodes'] = array(
    '#type' => 'checkbox',
    '#title' => t('Nodes'),
    '#default_value' => variable_get('i18n_select_nodes', TRUE),
  );
  $form['selection']['i18n_select_blocks'] = array(
    '#type' => 'checkbox',
    '#title' => t('Blocks (not implemented)'),
    '#default_value' => variable_get('i18n_select_blocks', TRUE),
  );
  $form['selection']['i18n_select_taxonomy'] = array(
    '#type' => 'checkbox',
    '#title' => t('Taxonomy'),
    '#default_value' => variable_get('i18n_select_taxonomy', TRUE),
  );

  return system_settings_form($form);
}

