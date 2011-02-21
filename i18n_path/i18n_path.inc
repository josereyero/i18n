<?php
// $Id: i18n_path.inc,v 1.1.2.1 2011/02/19 17:50:42 jareyero Exp $
/**
 * @file
 * Internationalization (i18n) module - Translation set
 */
class i18n_path_translation_set extends i18n_translation_set {
  /**
   * Clean path translations.
   * 
   * Unlike other translation sets this actually deletes paths
   */
  public function clean_translations() {
    $delete = db_delete('i18n_path')
      ->condition('tsid', $this->tsid)
      ->condition('language', array_keys($this->get_translations()), 'NOT IN')
      ->execute();
  }
  /**
   * Delete translation set
   */
  public function delete_translations() {
    return db_delete('i18n_path')
      ->condition('tsid', $this->tsid)
      ->execute();
  }
  /**
   * Save all path translations
   */
  public function save_translations() {
    foreach ($this->translations as $lang => $path) {
      $path = is_object($path) ? $path : (object)array('path' => $path, 'language' => $lang, 'tsid' => $this->tsid);
      drupal_write_record('i18n_path', $path, !empty($path->tpid) ? 'tpid' : array());
      $this->translations[$lang] = $path;
    }
  }
  /**
   * Get title from item
   */
  protected function item_title($item) {
    return $item['path'];
  }
  /**
   * Get link for item
   */
  protected function item_path($item) {
    return check_url($item['path']);
  }
}