<?php

/**
 * Define this Export UI plugin.
 */
$plugin = array(
  'schema' => 'multifield',
  'access' => 'administer multifield',
  'menu' => array(
    'menu prefix' => 'admin/structure',
    'menu item' => 'multifield',
    'menu title' => 'Multifield',
    'menu description' => 'Add, edit, and remove multifields.',
  ),
  'title singular' => t('multifield type'),
  'title singular proper' => t('Multifield type'),
  'title plural' => t('multifield types'),
  'title plural proper' => t('Multifield types'),
  'form' => array(
    'settings' => 'multifield_edit_form',
    //'validate' => 'redmine_server_edit_form_validate',
    //'submit' => 'redmine_server_edit_form_submit',
  ),
  'export' => array(
    'admin_title' => 'label',
  ),
   'handler' => array(
     'class' => 'multifield_export_ui',
     'parent' => 'ctools_export_ui',
   ),
   //'allowed operations' => array('add', 'edit'),
);

function multifield_edit_form(&$form, &$form_state) {
  $multifield = $form_state['item'];

  $form['info']['label']['#title'] = t('Multifield type label');
  $form['info']['machine_name']['#machine_name']['exists'] = 'multifield_field_machine_name_exists';
  $form['info']['machine_name']['#maxlength'] = '32';
  $form['info']['machine_name']['#title'] = t('Field type machine name');
  $form['info']['machine_name']['#machine_name']['label'] = t('Field type machine name');

  $form['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Description'),
    '#default_value' => $multifield->description,
  );
}
