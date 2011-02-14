<?php
// $Id: i18n_sync.node.inc,v 1.1.2.1 2011/02/14 11:36:29 jareyero Exp $

/**
 * @file
 * Internationalization (i18n) package. Synchronization of translations
 * 
 * Node synchronization.
 */

/**
 * Synchronizes fields for node translation.
 *
 * There's some specific handling for known fields like:
 * - files, for file attachments.
 * - iid (CCK node attachments, translations for them will be handled too).
 *
 * All the rest of the fields will be just copied over.
 * The 'revision' field will have the special effect of creating a revision too for the translation.
 *
 * @param $node
 *   Source node being edited.
 * @param $translations
 *   Node translations to synchronize, just needs nid property.
 * @param $fields
 *   List of fields to synchronize.
 * @param $op
 *   Node operation (insert|update).
 */
function i18n_sync_node_translation($node, $translations, $fields, $op) {
  $count = 0;
  // Disable language selection and synchronization temporarily, enable it again later
  i18n_select(FALSE);
  i18n_sync(FALSE);
  foreach ($translations as $translation) {
    // If translation is the same node, we cannot synchronize with itself
    if ($node->nid == $translation->nid) {
      continue;
    }
    // Load full node, we need all data here.
    $translation = node_load($translation->nid, NULL, TRUE);
    $field_info = i18n_sync_node_options($node->type);
    // Invoke callback for each field, the default is just copy over
    foreach ($fields as $field) {
      if (!empty($field_info[$field]['callback'])) {
        call_user_func($field_info[$field]['callback'], $node, $translation, $field);
      }
      elseif (!empty($field_info[$field]['field'])) {
        i18n_sync_node_translation_field_field($node, $translation, $field, $field_info[$field]['field']);
      }
    }
    // Give a chance to other modules for additional sync
    module_invoke_all('i18n_sync_node_translation', $node, $translation, $fields);
    node_save($translation);
    $count++;
  }
  i18n_sync(TRUE);
  i18n_select(TRUE);  
  drupal_set_message(format_plural($count, 'One node translation has been synchronized.', 'All @count node translations have been synchronized.'));
}

/**
 * Synchronize default field
 */
function i18n_sync_node_translation_default_field($node, $translation, $field) {
  switch ($field) {
    case 'parent': // Book outlines, translating parent page if exists.
    case 'iid': // Attached image nodes.
      i18n_sync_node_translation_attached_node($node, $translation, $field);
      break;

    case 'files':
      // Sync existing attached files. This should work for images too
      foreach ((array)$node->files as $fid => $file) {
        if (isset($translation->files[$fid])) {
          // Just update list and weight properties, description can be different
          $translation->files[$fid]->list = $file->list;
          $translation->files[$fid]->weight = $file->weight;
        }
        else {
          // New file. Clone so we can set the new property just for this translation
          $translation->files[$fid] = clone $file;
          $translation->files[$fid]->new = TRUE;
        }
      }
      // Drop removed files.
      foreach ((array)$translation->files as $fid => $file) {
        if (!isset($node->files[$fid])) {
          $translation->files[$fid]->remove = TRUE;
        }
      }
      break;

    default:
      // For fields that don't need special handling.
      if (isset($node->$field)) {
        $translation->$field = $node->$field;
      }
  }  
}


/**
 * Synchronize taxonomy.
 *
 * Translate translatable terms, just copy over the rest.
 */
function i18n_sync_node_translation_taxonomy(&$node, &$source) {
  if (module_exists('i18n_taxonomy') && is_array($source->taxonomy)) {
    // Load clean source node taxonomy so we don't need to handle weird form input
    if (!isset($source->i18n_taxonomy)) {
      $source->i18n_taxonomy = i18n_taxonomy_node_get_terms($source);
    }
    $node->taxonomy = i18n_taxonomy_translate_terms($source->i18n_taxonomy, $node->language, FALSE);
  }
  else {
    // If not multilingual taxonomy enabled, just copy over.
    $node->taxonomy = $source->taxonomy;
  }
}

/**
 * Node attachments (CCK) that may have translation.
 */
function i18n_sync_node_translation_attached_node(&$node, &$translation, $field) {
  if ($attached = node_load($node->$field)) {
    $translation->$field = i18n_sync_node_translation_reference_field($attached, $node->$field, $translation->language);
  }
}

/**
 * Translating a nodereference field (cck).
 */
function i18n_sync_node_translation_nodereference_field(&$node, &$translation, $field) {
  $translated_references = array();
  foreach ($node->$field as $reference) {
    if ($reference_node = node_load($reference['nid'])) {
      $translated_references[] = array(
        'nid' => i18n_sync_node_translation_reference_field($reference_node, $reference['nid'], $translation->language)
      );
    }
  }
  $translation->$field = $translated_references;
}

/**
 * Translating an filefield (cck).
 */
function i18n_sync_node_translation_filefield_field(&$node, &$translation, $field) {
  if (is_array($node->$field)) {
    $translated_images = array();
    foreach ($node->$field as $file) {
      $found = false;

      // Try to find existing translations of the filefield items and reference them.
      foreach ($translation->$field as $translation_image) {
        if ($file['fid'] == $translation_image['fid']) {
          $translated_images[] = $translation_image;
          $found = true;
        }
      }

      // If there was no translation found for the filefield item, just copy it.
      if (!$found) {
        $translated_images[] = $file;
      }
    }
    $translation->$field = $translated_images;
  }
}

/**
 * Helper function to which translates reference field. We try to use translations for reference, otherwise fallback.
 * Example:
 *   English A references English B and English C.
 *   English A and B are translated to German A and B, but English C is not.
 *   The syncronization from English A to German A would it German B and English C.
 */
function i18n_sync_node_translation_reference_field(&$reference_node, $default_value, $langcode) {
  if (isset($reference_node->tnid) && translation_supported_type($reference_node->type)) {
    // This content type has translations, find the one.
    if (($reference_trans = translation_node_get_translations($reference_node->tnid)) && isset($reference_trans[$langcode])) {
      return $reference_trans[$langcode]->nid;
    }
    else {
      // No requested language found, just copy the field.
      return $default_value;
    }
  }
  else {
    // Content type without language, just copy the field.
    return $default_value;
  }
}

/**
 * Synchronize configurable field
 * 
 * @param $field_info
 *   Field API field information.
 */
function i18n_sync_node_translation_field_field($node, $translation, $field, $field_info) {
  switch ($field_info['type']) {
    case 'taxonomy_term_reference':
      // Do nothing it has already been syncd.
      // i18n_sync_node_translation_taxonomy($translation, $node);
      break;
    default:
      // For fields that don't need special handling, just copy it over if defined.
      if (isset($node->$field)) {
        $translation->$field = $node->$field;
      }      
  } 
}