<?php
/**
 * @file
 * Variable information
 */

/**
 * Implements hook_variable_group_info()
 */
function i18n_variable_group_info() {
  $groups['i18n'] = array(
    'title' => t('Multilingual settings'),
    'description' => t('Mixed options for multilingual sites.'),
    'access' => 'administer site configuration',
    'path' => 'admin/config/regional/i18n',     
  );
  return $groups;
}