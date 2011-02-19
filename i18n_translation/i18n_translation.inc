<?php
// $Id: i18n_translation.inc,v 1.1.2.1 2011/02/19 17:50:42 jareyero Exp $
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
   * Clean all terms in this translation set
   */
  public function clean_translations() {
  }
  /**
   * Save translations in this translation set
   */
  public function save_translations() {
  }
  /**
   * Delete translations
   */
  public function delete_translations() {
  }
  /**
   * Get translations
   */
  public function get_translations() {
    if (!isset($this->translations)) {
      $this->translations = $this->load_translations();
    }
    return $this->translations;
  }
  /**
   * Add translations
   */
  public function add_translations($translations) {
    foreach ($translations as $object) {
      $this->translations[$object->language] = $object;
    }
  }
  /**
   * Load all translations
   */
  public function load_translations() {
    return array();
  }
}