<?php
// $Id: i18n_select.admin.inc,v 1.1.2.3 2011/02/14 11:36:29 jareyero Exp $

function i18n_select_admin_settings() {
  $form['mode'] = array(
    '#type' => 'fieldset',
    '#title' => t('Content selection mode'),
  );
  $form['mode']['i18n_select_missing_translation'] = array(
    '#type' => 'checkbox',
    '#title' => t('When translation is missing, show content in the default language'),
    '#default_value' => variable_get('i18n_select_missing_translation', FALSE),
  );

  $form['types'] = array(
    '#type' => 'fieldset',
    '#title' => t('Content to filter by language'),
  );
  $form['types']['i18n_select_nodes'] = array(
    '#type' => 'checkbox',
    '#title' => t('Nodes'),
    '#default_value' => variable_get('i18n_select_nodes', TRUE),
  );
  $form['types']['i18n_select_blocks'] = array(
    '#type' => 'checkbox',
    '#title' => t('Blocks (not implemented)'),
    '#default_value' => variable_get('i18n_select_blocks', TRUE),
  );
  $form['types']['i18n_select_taxonomy'] = array(
    '#type' => 'checkbox',
    '#title' => t('Taxonomy'),
    '#default_value' => variable_get('i18n_select_taxonomy', TRUE),
  );
  // Enable for specific pages. This works pretty much like block visibility
  // Note this page requires 'administer site configuration' so we don't need to check permissions
  $form['pages'] = array(
    '#type' => 'fieldset',
    '#title' => t('Enable for specific pages'),
  );
  $options = array(
    I18N_SELECT_PAGE_NOTLISTED => t('All pages except those listed'),
    I18N_SELECT_PAGE_LISTED => t('Only the listed pages'),
  );
  $description = t("Specify pages by using their paths. Enter one path per line. The '*' character is a wildcard. Example paths are %blog for the blog page and %blog-wildcard for every personal blog. %front is the front page.", array('%blog' => 'blog', '%blog-wildcard' => 'blog/*', '%front' => '<front>'));
  if (module_exists('php')) {
    $options += array(I18N_SELECT_PAGE_PHP => t('Pages on which this PHP code returns <code>TRUE</code> (experts only)'));
    $title = t('Pages or PHP code');
    $description .= ' ' . t('If the PHP option is chosen, enter PHP code between %php. Note that executing incorrect PHP code can break your Drupal site.', array('%php' => '<?php ?>'));
  }
  else {
    $title = t('Pages');
  }  
  $form['pages']['i18n_select_page_mode'] = array(
    '#type' => 'radios',
    '#options' => $options,
    '#default_value' => variable_get('i18n_select_page_mode', I18N_SELECT_PAGE_NOTLISTED),
  );
  $form['pages']['i18n_select_page_list'] = array(
    '#type' => 'textarea',
    '#title' => '<span class="element-invisible">' . $title . '</span>',
    '#default_value' => variable_get('i18n_select_page_list', 'admin/*'),
    '#description' => $description,
  );
  $form['pages']['i18n_select_page_block'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable always for block content though it may be disabled for the page'),
    '#default_value' => variable_get('i18n_select_page_block', TRUE),
  );
  return system_settings_form($form);
}

