<?php
// $Id: i18n_taxonomy.inc,v 1.1.2.1 2011/02/19 17:50:42 jareyero Exp $
/**
 * @file
 * Internationalization (i18n) module - Translation set
 */
class i18n_taxonomy_translation_set extends i18n_translation_set {
  /**
   * Load all path translations
   */
  public function load_translations() {
    return i18n_translation_set_index(taxonomy_term_load_multiple(array(), array('i18n_tsid' => $this->tsid)));
  }
}