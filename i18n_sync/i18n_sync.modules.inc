<?php
// $Id: i18n_sync.module,v 1.1.2.35 2010/03/04 20:11:10 jareyero Exp $

/**
 * @file
 * Internationalization (i18n) package. Synchronization of translations
 * 
 * Implements hook_i18n_sync_node_fields() for several core and contrib modules.
 */

/**
 * Comment module
 */
function comment_i18n_sync_node_fields($type) {
  $fields['comment'] = array('title' =>  t('Comment settings'));
  return $fields;
}

/**
 * Event module
 */
function event_i18n_sync_node_fields($type) {
  // Event fields.
  if ($type && variable_get('event_nodeapi_'. $type, 'never') != 'never') {
    $fields['event_start'] = array('title' => t('Event start'), 'group' => 'event');
    $fields['event_end'] = array('title' => t('Event end'), 'group' => 'event');
    $fields['timezone'] = array('title' => t('Timezone'), 'group' => 'event');       
    return $fields;
  }  
}

/**
 * Field module
 */
function field_i18n_sync_node_fields($type) {
  if ($type) {
    $sync_fields = array();
    $instances = field_info_instances('node', $type);
    $field_types = field_info_field_types();
    foreach ($instances as $name => $instance) {
      $field = field_info_field($instance['field_name']);
      $sync_fields[$name] = array(
        'title' => $instance['label'],
        'description' => $instance['description'],
        'field_type' => $field['type'],
        'group' => 'fields',
      );
    }
    return $sync_fields;
  }
}

/**
 * Image module
 */
function image_i18n_sync_node_fields($type) {
  $fields = array();
  // Image and image attach.
  if ($type == 'image') {
    $fields['images'] = array('title' => t('Image files'));
  }
  if ($type && module_exists('image_attach') && variable_get('image_attach_'. $type, 0)) {
    $fields['iid'] = array('title' => t('Attached image nodes'));
  }
  return $fields;
}

/**
 * Location module
 */
function location_i18n_sync_node_fields($type) {
  $fields['locations'] = array('title' => t('Location information'));
  return $fields;
}
