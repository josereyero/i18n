<?php
// $Id$

/**
 * Implements hook_features_pipe_alter() for node component.
 * Add i18n_node variables which are related to this node type
 */
function i18n_node_features_pipe_node_alter(&$pipe, $data, $export) {
  if (!empty($data)) {
    $variables = array(
      'i18n_node',
      'i18n_newnode_current',
      'i18n_required_node',
      'i18n_lock_node',
    );
    foreach ($data as $node_type) {
      foreach ($variables as $variable_name) {
        $pipe['variable'][] = "{$variable_name}_{$node_type}";
      }
    }
  }
}