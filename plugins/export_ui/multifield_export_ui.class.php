<?php

class multifield_export_ui extends ctools_export_ui {
  function access($op, $item) {
    if (in_array($op, array('import', 'export', 'revert', 'disable', 'enable'))) {
      return FALSE;
    }
    return parent::access($op, $item);
  }

  function build_operations($item) {
    // WTF, why doesn't ctools_export_ui() run these operations
    // through the access method???
    $allowed_operations = parent::build_operations($item);
    $i = 0;
    foreach ($allowed_operations as $op => $operation) {
      if (!$this->access($op, $item)) {
        unset($allowed_operations[$op]);
      }
      else {
        $allowed_operations[$op]['weight'] = $i++;
      }
    }

    $has_instances = multifield_type_has_instances($item->machine_name);
    $allowed_operations['manage fields'] = array(
      'title' => t('Manage fields'),
      'href' => 'admin/structure/multifield/list/' . $item->machine_name . '/fields',
      'weight' => $has_instances ? -5 : -10,
    );
    $allowed_operations['manage display'] = array(
      'title' => t('Manage display'),
      'href' => 'admin/structure/multifield/list/' . $item->machine_name . '/display',
      'weight' => $has_instances ? -10 : -5,
    );
    uasort($allowed_operations, 'drupal_sort_weight');

    return $allowed_operations;
  }

  function list_form(&$form, &$form_state) {
    parent::list_form($form, $form_state);
    $form['top row']['disabled']['#access'] = FALSE;
  }

  function delete_page($js, $input, $item) {
    $output = parent::delete_page($js, $input, $item);

    if ($fields = multifield_type_get_fields($item)) {
      $output['description']['#markup'] .= ' ' . t('The following fields and their respective data will also be removed, which cannot be undone:') . theme('item_list', array('items' => $fields));
    }

    return $output;
  }
}
