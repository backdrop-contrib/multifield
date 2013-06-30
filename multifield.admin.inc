<?php

function multifield_list_page() {
  $multifields = multifield_load_all();
  $fields = multifield_type_get_fields();
  $rows = array();
  foreach ($multifields as $multifield) {
    $locked = !empty($fields[$multifield->machine_name]);
    $row = array(
      check_plain($multifield->label),
      !empty($multifield->description) ? check_plain(truncate_utf8($multifield->description, 32)) : t('None'),
      check_plain($multifield->machine_name),
      $locked ? t('Locked for field editing') : t('Unlocked'),
    );
    $operations = array();
    $operations['edit'] = array(
      'title' => t('Edit info'),
      'href' => 'admin/structure/multifield/manage/' . $multifield->machine_name,
    );
    if (!$locked) {
      $operations['field'] = array(
        'title' => t('Manage fields'),
        'href' => 'admin/structure/multifield/manage/' . $multifield->machine_name . '/fields',
      );
    }
    $operations['display'] = array(
      'title' => t('Manage display'),
      'href' => 'admin/structure/multifield/manage/' . $multifield->machine_name . '/display',
    );
    $operations['delete'] = array(
      'title' => t('Delete'),
      'href' => 'admin/structure/multifield/manage/' . $multifield->machine_name . '/delete',
    );
    $row[] = theme('links', array('links' => $operations, 'attributes' => array('class' => 'links inline')));

    $rows[] = $row;
  }

  $build['list'] = array(
    '#theme' => 'table',
    '#header' => array(
      'label' => t('Label'),
      'description' => t('Description'),
      'machine_name' => t('Machine name'),
      'status' => t('Status'),
      'operations' => t('Operations'),
    ),
    '#rows' => $rows,
    '#empty' => t('No multifield types available.'),
  );

  return $build;
}

function multifield_edit_form($form, &$form_state, $multifield = NULL) {
  if (!isset($multifield)) {
    $multifield = new stdClass();
    $multifield->label = NULL;
    $multifield->machine_name = NULL;
    $multifield->description = NULL;
  }


  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#default_value' => $multifield->label,
    '#required' => TRUE,
  );

  $form['machine_name'] = array(
    '#type' => 'machine_name',
    '#title' => t('Machine name'),
    '#default_value' => $multifield->machine_name,
    '#disabled' => !empty($multifield->machine_name),
    '#required' => TRUE,
    '#maxlength' => 32,
    '#machine_name' => array(
      'exists' => 'multifield_field_machine_name_exists',
      'source' => array('label'),
      'label' => t('Field type machine name'),
    ),
  );

  $form['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Description'),
    '#default_value' => $multifield->description,
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['actions']['cancel'] = array(

  );

  return $form;
}
