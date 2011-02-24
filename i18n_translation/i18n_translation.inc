<?php
// $Id: i18n_translation.inc,v 1.1.2.2 2011/02/24 09:08:59 jareyero Exp $
/**
 * @file
 * Internationalization (i18n) module - Translation set
 */
class i18n_translation_set {
  public $tsid = NULL;
  public $type;
  public $bundle = '';
  public $status = 0;
  public $master_id = 0;
  // Translations indexed by language
  public $translations = NULL;
  
  /**
   * Constructor from object/array
   */
  public function __construct($translation = NULL) {
    if ($translation) {
      foreach ((array)$translation as $key => $value) {
        $this->$key = $value;
      }
    }
  }

  /**
   * Delete a translation set
   */
  public function delete($delete_translations = FALSE) {
    db_delete('i18n_translation_set')
      ->condition('tsid', $this->tsid)
      ->execute();
    if ($delete_translations) {
      $this->delete_translations();
    }
    $this->invoke_all('delete');
    $this->tsid = NULL;
  }

  /**
   * Invoke all modules
   */
  public function invoke_all($op) {
    module_invoke_all('i18n_translation_set_' . $op, $this);
    module_invoke_all('entity_' . $op, $this, 'i18n_translation');
  }

  /**
   * Create a new translation set
   */
  public function insert($save_translations = FALSE) {
    $this->created = $this->changed = REQUEST_TIME;
    $status = drupal_write_record('i18n_translation_set', $this);
    if ($save_translations) {
      $this->save_translations();
    }
    $this->invoke_all('insert');
    return $status;
  }

  /**
   * Save translation set
   */
  public function save($save_translations = FALSE) {
    $this->invoke_all('presave');
    return empty($this->tsid) ? $this->insert($save_translations) : $this->update($save_translations);
  }

  /**
   * Create a new translation set
   */
  public function update($udpate_translations = FALSE) {
    $this->changed = REQUEST_TIME;
    $status = drupal_write_record('i18n_translation_set', $this, 'tsid');
    if ($udpate_translations) {
      $this->clean_translations();
      $this->save_translations();
    }
    $this->invoke_all('update');
    return $status;
  }

  /**
   * Clean all items in this translation set
   * 
   * Unlink other items (not current translations from this translation set)
   */
  public function clean_translations() {
    list($table, $field) = $this->table_field();
    if ($table && $field) {
      $query = db_update($table)
        ->fields(array($field => 0))
        ->condition($table . '.' . $field, $this->tsid);
      if ($translations = $this->get_translations()) {  
        $query->condition('language', array_keys($translations), 'NOT IN');
      }
      return $query->execute();
    }
  }
  /**
   * Save translations in this translation set
   */
  public function save_translations() {
    list($table, $field) = $this->table_field();
    if ($table && $field) {
      if ($keys = $this->get_translations_keys()) {
        return db_update($table)
          ->fields(array($field => $this->tsid))
          ->condition($this->key_field(), $keys)
          ->execute();
      }
      else {
        return $this->delete_translations();
      }
    }
    
  }
  /**
   * Delete translations in this translation set
   * 
   * It won't delete objects, just unlink them from translation set
   */
  public function delete_translations() {
    list($table, $field) = $this->table_field();
    if ($table && $field) {
      return db_update($table)
        ->fields(array($field => 0))
        ->condition($field, $this->tsid)
        ->execute();
    }
  }
  /**
   * Get translations, indexed by language
   */
  public function get_translations() {
    if (!isset($this->translations)) {
      $this->translations = $this->load_translations();
    }
    return $this->translations;
  }
  /**
   * Get translations keys, indexed by language
   */
  public function get_translations_keys() {
    $keys = array();
    foreach ($this->get_translations() as $lang => $item) {
      if ($id = i18n_object_field($item, $this->key_field())) {
        $keys[$lang] = $id;
      }
    }
    return array_filter($keys);
  }
  /**
   * Get edit path for this translation set
   */
  public function get_edit_path() {
    if ($path = $this->get_info('edit path')) {
      return $path . '/' . $this->tsid;
    }
    else {
      return '';
    }
  }
  /**
   * Get property from translation set info
   */
  public function get_info($property, $default = NULL) {
    $info = i18n_translation_set_info($this->type);
    return $info && isset($info['translation set'][$property]) ? $info['translation set'][$property] : $default;
  }
  /**
   * Get title for this set
   */
  public function get_title() {
    if (!empty($this->title)) {
      $title = $this->title;
    }
    elseif ($translations = $this->get_translations()) {
      foreach ($translations as $item) {
        $names[] = $this->item_title($item);
      }
      return implode(' / ', $names);
    }
    else {
      return t('Undefined');
    }
  }
  /**
   * Get title from item
   */
  protected function item_title($item) {
    return entity_label($this->type, $item);
  }
  /**
   * Get link for item
   */
  protected function item_path($item) {
    return entity_uri($this->type, $item);
  }
  /**
   * Get item list
   */
  public function item_list() {
    $items = array();
    foreach ($this->get_translations() as $langcode => $item) {
      $title = $this->item_title($item);
      $path = $this->item_path($item);
      if ($title && $path) {
        $items[$langcode] = l($title, $path);
      }
      elseif ($title) {
        $items[$langcode] = check_plain($title);
      }
    }
    return $items;
  }
  /**
   * Add translations
   */
  public function add_translations($translations) {
    array_map(array($this, 'add_item'), $translations);
  }
  /**
   * Add translation item
   */
  public function add_item($item, $langcode = NULL) {
    $item = $this->check_item($item);
    $langcode = $langcode ? $langcode : i18n_object_langcode($item);
    if ($item && $langcode) {
      $this->translations[$langcode] = $item;
      return TRUE;
    }
    else {
      return FALSE;
    }
  }
  /**
   * Check item has translation set property
   */
  public function check_item($item) {
    list($table, $field) = $this->table_field();
    if (is_object($item)) {
      $item->$field = $this->tsid;
    }
    elseif (is_array($item)) {
      $item[$field] = $this->tsid;
    }
    return $item;
  }
  /**
   * Load all translations as objects indexed by language
   */
  public function load_translations() {
    list($table, $field) = $this->table_field();
    if ($table && $field) {
      return db_select($table)
        ->fields($table)
        ->condition($table . '.' . $field, $this->tsid)
        ->execute()
        ->fetchAllAssoc('language');
    }
    else {
      return array();
    }
  }

  /**
   * Get table / field for this translation type
   */
  protected function table_field() {
    return array($this->get_info('table'), $this->get_info('field'));
  }
  /**
   * Get key field for this translation items
   */
  protected function key_field() {
    $info = i18n_object_info($this->type);
    return $info['key'];    
  }
}