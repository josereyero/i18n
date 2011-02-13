<?php
// $Id: i18n_sync.module,v 1.1.2.35 2010/03/04 20:11:10 jareyero Exp $

/**
 * @file
 * Internationalization (i18n) package. Synchronization of translations
 * 
 * Implements hook_i18n_sync_node_fields() for several core and contrib modules.
 */

/**
 * Book module. Implements hook_i18n_sync_options().
 */
function book_i18n_sync_options($entity_type, $bundle_name) {
  if ($entity_type == 'node') {
    return array(
      'parent' => array('title' =>t('Book outline'), 'description' => t('Set the translated parent for each translation if possible.')),
    );
  }
}

/**
 * Comment module. Implements hook_i18n_sync_options().
 */
function comment_i18n_sync_options($entity_type, $bundle_name) {
  if ($entity_type == 'node') {
    $fields['comment'] = array('title' =>  t('Comment settings'));
    return $fields;
  }
}

/**
 * Field module. Implements hook_i18n_sync_options().
 */
function field_i18n_sync_options($entity_type, $bundle_name) {
  $sync_fields = array();
  if ($bundle_name) {
    $instances = field_info_instances($entity_type, $bundle_name);
    foreach ($instances as $name => $instance) {
      $field = field_info_field($instance['field_name']);
      $sync_fields[$name] = array(
        'title' => $instance['label'],
        'description' => $instance['description'],
        'field' => $field,
        'group' => 'fields',
      );
    }
  }
  return $sync_fields;
}

/**
 * Node module. Implements hook_i18n_sync_options().
 */
function node_i18n_sync_options($entity_type, $bundle_name) {
  if ($entity_type == 'node') {
    return array(
      'name' => array('title' => t('Author')),
      'status' => array('title' => t('Status')),
      'promote' => array('title' => t('Promote')),
      'moderate' =>  array('title' => t('Moderate')),
      'sticky' => array('title' => t('Sticky')),
      'revision' =>  array('title' => t('Revision'), 'description' => t('Create also new revision for translations')),
    );
  }
}
