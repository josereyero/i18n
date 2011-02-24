<?php
// $Id: i18n_menu.inc,v 1.1.2.1 2011/02/24 09:08:59 jareyero Exp $
/**
 * @file
 * Internationalization (i18n) module - Translation set
 */
class i18n_menu_link_translation_set extends i18n_translation_set {
  /**
   * Get title from item
   */
  protected function item_title($item) {
    return $item['title'];
  }
  /**
   * Get link for item
   */
  protected function item_path($item) {
    return $item['link_path'];
  }
  /**
   * Load all path translations
   */
  public function load_translations() {
    $translations = array();
    $query = db_select('menu_links', 'ml');
    $query->leftJoin('menu_router', 'm', 'm.path = ml.router_path');
    $query->fields('ml');
    // Weight should be taken from {menu_links}, not {menu_router}.
    $query->addField('ml', 'weight', 'link_weight');
    $query->fields('m');
    $query->condition('ml.i18n_tsid', $this->tsid);
    foreach ($query->execute()->fetchAll(PDO::FETCH_ASSOC) as $item) {
      $item['weight'] = $item['link_weight'];
      _menu_link_translate($item);
      $translations[$item['language']] = $item;
    }
    return $translations;
  }
}