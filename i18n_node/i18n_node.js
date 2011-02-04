// $Id: i18n.js,v 1.1.2.3 2009/01/20 20:35:55 nedjo Exp $
(function ($) {

/**
 * Rewrite autocomplete inputs to pass the language of the node currently being
 * edited in the path.
 *
 * This functionality ensures node autocompletes get suggestions for the node's
 * language rather than the current interface language.
 */
Drupal.behaviors.i18n = {
  attach : function (context) {
    if (Drupal.settings && Drupal.settings.i18n) {
      $('form[id^=node-form]', context).find('input.autocomplete[value^=' + Drupal.settings.i18n.interface_path + ']').each(function () {
        $(this).val($(this).val().replace(Drupal.settings.i18n.interface_path, Drupal.settings.i18n.content_path));
      });
    }
  }
};

})(jQuery);
