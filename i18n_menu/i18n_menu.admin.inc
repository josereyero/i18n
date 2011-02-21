<?php
// $Id: i18n_taxonomy.admin.inc,v 1.1.2.3 2011/02/14 11:36:29 jareyero Exp $

/**
 * @file
 * Helper functions for menu administration.
 */

/**
 * Generate a tabular listing of translations for vocabularies.
 */
function i18n_menu_translation_overview($menu) {
  drupal_set_title(check_plain($menu['menu_name']));
  $output = '';

  if ($rows) {
    $build['translations'] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows
    );
  }
  else {
    $build['message']['#markup'] = t('No translations defined for this menu.');
  }
  return $build;
}

/**
 * Produces a vocabulary translation form.
 */
function i18n_menu_translation_form($form, $form_state, $menu = NULL, $item = NULL, $translation_set = NULL) {
  $form['translation_set'] = array('#type' => 'value', '#value' => $translation_set);
  $translations = $translation_set ? $translation_set->get_translations() : array();
  if ($item && ($lang = i18n_object_langcode($item))) {
    $translations[$lang] = $item;
  }
  $item = $item ? $item : array('mlid' => 0, 'menu_name' => '', 'plid' => 0);

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
    '#description' => t('Enter items that will be considered as translations of each other.'),
  );
  foreach (i18n_language_list() as $langcode => $language_name) {
    // Generate a list of possible parents (not including this link or descendants).
    $options = i18n_menu_parent_options(menu_get_menus(), $item, $langcode);
    $default = isset($translations[$langcode]) ? $translations[$langcode]['menu_name'] . ':' . $translations[$langcode]['mlid'] : 'navigation:0';
    if (!isset($options[$default])) {
      $default = 'navigation:0';
    }
    $form['translations'][$langcode] = array(
      '#type' => 'select',
      '#title' => $language_name,
      '#default_value' => $default,
      '#options' => $options,
      '#description' => t('The maximum depth for a link and all its children is fixed at !maxdepth. Some menu links may not be available as parents if selecting them would exceed this limit.', array('!maxdepth' => MENU_MAX_DEPTH)),
      '#attributes' => array('class' => array('menu-title-select')),  
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
function i18n_menu_translation_form_validate($form, &$form_state)  {
  if ($form_state['values']['op'] == t('Save') && !array_filter($form_state['values']['translations'])) {
    form_set_error('paths', t('There are no path translations to save.'));
  }
}

/**
 * Menu item translation form submission
 */
function i18n_menu_translation_form_submit($form, &$form_state) {
  $translation_set = $form_state['values']['translation_set'];

  switch ($form_state['values']['op']) {
    case t('Save'):
      $mlids = array_filter($form_state['values']['translations']);
      $translation_set = $translation_set ? $translation_set : i18n_translation_set_create('menu_link');
      $translations = $translation_set->get_translations();
      $translation_set->translations = array();
      foreach ($mlids as $lang => $item_name) {
        list($menu_name, $mlid) = explode(':', $item_name);
        $translation_set->translations[$lang] = menu_link_load($mlid);
      }
      $translation_set->save(TRUE);
      drupal_set_message(t('The item translation has been saved.'));
      break;
    case t('Delete'):
      $translation_set->delete(TRUE);
      drupal_set_message(t('The item translation has been deleted.'));
      break;
  }
  $form_state['redirect'] = 'admin/structure/menu';
}

/**
 * Return a list of menu items that are valid possible parents for the given menu item.
 *
 * @param $menus
 *   An array of menu names and titles, such as from menu_get_menus().
 * @param $item
 *   The menu item or the node type for which to generate a list of parents.
 *   If $item['mlid'] == 0 then the complete tree is returned.
 * @return
 *   An array of menu link titles keyed on the a string containing the menu name
 *   and mlid. The list excludes the given item and its children.
 *
 * @todo This has to be turned into a #process form element callback. The
 *   'menu_override_parent_selector' variable is entirely superfluous.
 */
function i18n_menu_parent_options($menus, $item, $langcode) {
  // The menu_links table can be practically any size and we need a way to
  // allow contrib modules to provide more scalable pattern choosers.
  // hook_form_alter is too late in itself because all the possible parents are
  // retrieved here, unless menu_override_parent_selector is set to TRUE.
  if (variable_get('i18n_menu_override_parent_selector', FALSE)) {
    return array();
  }
  // If no menu item, create a dummy one
  $item = $item ? $item : array('mlid' => 0);
  // Get menus that have a language or have language for terms
  $available_menus = array();
  foreach (menu_load_all() as $name => $menu) {
    if ($menu['i18n_mode'] & I18N_MODE_TRANSLATE) {
      $available_menus[$name] = $menu;
    }
    elseif ($menu['i18n_mode'] & I18N_MODE_LANGUAGE && $menu['language'] == $langcode) {
      $available_menus[$name] = $menu;
    }
  }

  $options = _i18n_menu_get_options($menus, $available_menus, $item, $langcode);
  return $options;
}

/**
 * Helper function to get the items of the given menu.
 */
function _i18n_menu_get_options($menus, $available_menus, $item, $langcode) {
  // If the item has children, there is an added limit to the depth of valid parents.
  if (isset($item['parent_depth_limit'])) {
    $limit = $item['parent_depth_limit'];
  }
  else {
    $limit = _menu_parent_depth_limit($item);
  }

  $options = array();
  foreach ($menus as $menu_name => $title) {
    if (isset($available_menus[$menu_name])) {
      $tree = i18n_menu_tree_all_data($menu_name, $langcode, NULL);
      $options[$menu_name . ':0'] = '<' . $title . '>';
      _menu_parents_recurse($tree, $menu_name, '--', $options, $item['mlid'], $limit);
    }
  }
  return $options;
}

/**
 * Filter out menu items that have a different language
 */
function i18n_menu_tree_all_data($menu_name, $langcode, $link = NULL, $max_depth = NULL) {
  $tree = menu_tree_all_data($menu_name, $link, $max_depth);
  foreach ($tree as $key => $item) {
    if (!empty($item['link']['customized']) && ($lang = i18n_object_langcode($item['link'])) && $lang != $langcode) {
      unset($tree[$key]);
    }
  }
  return $tree;
}