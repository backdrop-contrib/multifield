<?php
/**
 * @file
 * Field integration for the Multifield module.
 */

// Trying to keep track of all the relevant field hooks that need to be run.
//  field_attach_delete()
//  field_attach_form()
//  field_attach_form_validate()
//  field_attach_insert()
//  field_attach_load()
//  field_attach_load_revision()
//  field_attach_prepare_view()
//  field_attach_preprocess()
//  field_attach_presave()
//  field_attach_submit()
//  field_attach_update()
//X field_attach_validate()
//  field_attach_view()

/**
 * Implements hook_field_info().
 */
function multifield_field_info() {
  $info = array();

  $info['multifield'] = array(
    'label' => t('Multifield'),
    'default_widget' => 'multifield_default',
    'default_formatter' => 'multifield_default',
    'settings' => array(
      'hide_blank_items' => TRUE,
      'entity_translation_sync' => array('id'),
    ),
  );

  // Deprecated field types from the CTools exports.
  $multifields = multifield_load_all();
  foreach ($multifields as $machine_name => $multifield) {
    // Only show this multifield as an option to create if it has subfields.
    if (empty($multifield->locked) && multifield_type_has_subfields($machine_name)) {
      $info[$machine_name] = array(
        'label' => $multifield->label,
        'description' => $multifield->description,
        'default_widget' => 'multifield_default',
        'default_formatter' => 'multifield_default',
        'settings' => array(
          'hide_blank_items' => TRUE,
          'entity_translation_sync' => array('id'),
        ),
        'no_ui' => TRUE,
      );
    }
  }

  return $info;
}

/**
 * Implements hook_field_settings_form().
 */
function multifield_field_settings_form($field, $instance) {
  $form['hide_blank_items'] = array(
    '#type' => 'checkbox',
    '#title' => t('Hide blank items'),
    '#default_value' => $field['settings']['hide_blank_items'],
    '#description' => t("A blank item is always added to any multivalued field's form. If checked, any additional blank items are hidden except of the first item which is always shown."),
    '#weight' => 10,
    '#states' => array(
      // Show this setting only if the cardinality is unlimited.
      'visible' => array(
        ':input[name="field[cardinality]"]' => array('value' => (string) FIELD_CARDINALITY_UNLIMITED),
      ),
    ),
  );
  return $form;
}

/**
 * Implements hook_field_load().
 */
function multifield_field_load($entity_type, $entities, $field, $instances, $langcode, &$items, $age) {
  $machine_name = multifield_extract_multifield_machine_name($field);

  foreach ($entities as $id => $entity) {
    array_walk($items[$id], 'multifield_item_unserialize', $machine_name);
  }

  foreach (multifield_type_get_subfields($machine_name) as $subfield_name) {
    $subfield = field_info_field($subfield_name);
    // Once the subfields have been imploded into a proper structure, we need
    // to filter out 'empty' values.
    foreach (array_keys($entities) as $id) {
      foreach ($items[$id] as $delta => $item) {
        if (!empty($item[$subfield_name][LANGUAGE_NONE])) {
          $items[$id][$delta][$subfield_name][LANGUAGE_NONE] = _field_filter_items($subfield, $item[$subfield_name][LANGUAGE_NONE]);
        }
      }
    }
  }

  $pseudo_entities = array();
  foreach ($entities as $id => $entity) {
    foreach ($items[$id] as $delta => $item) {
      $item['parent'] = $entity;
      $pseudo_entity = _multifield_field_item_to_entity($machine_name, $item);
      $pseudo_entity->delta = $delta;
      $pseudo_entity->parent_id = $id;
      $pseudo_entities[$pseudo_entity->id] = $pseudo_entity;
    }
  }

  // @todo Invoke hook_field_storage_pre_load()?

  // Invoke field-type module's hook_field_load().
  $null = NULL;
  $options = array();
  _field_invoke_multiple('load', 'multifield', $pseudo_entities, $age, $null, $options);

  // Invoke hook_field_attach_load(): let other modules act on loading the
  // entity.
  // @todo Should this move to getting invoked from multifield_field_attach_load()?
  module_invoke_all('field_attach_load', 'multifield', $pseudo_entities, $age, $options);

  // Store the loaded multifield entities in the entity cache.
  entity_get_controller('multifield')->cacheSet($pseudo_entities);

  foreach ($pseudo_entities as $pseudo_entity) {
    $item = &$items[$pseudo_entity->parent_id][$pseudo_entity->delta];
    unset($pseudo_entity->delta);
    unset($pseudo_entity->parent_id);
    $item = _multifield_field_entity_to_item($pseudo_entity);
  }
}

/**
 * Implements hook_field_access().
 *
 * @todo Investigate improving performance of this function.
 */
function multifield_field_access($op, $field, $entity_type, $entity, $account) {
  // We should return FALSE if all the subfields' field_access() checks all
  // return FALSE.
  if ($machine_name = multifield_extract_multifield_machine_name($field)) {
    $subfield_names = multifield_type_get_subfields($machine_name);
    $subfield_access = array();

    if (!empty($entity) && $items = field_get_items($entity_type, $entity, $field['field_name'])) {
      foreach ($items as $delta => $item) {
        $item['parent'] = $entity;
        $pseudo_entity = _multifield_field_item_to_entity($machine_name, $item);
        foreach ($subfield_names as $subfield_name) {
          $subfield = field_info_field($subfield_name);
          $subfield_access[$subfield_name . ':' . $delta] = field_access($op, $subfield, 'multifield', $pseudo_entity, $account);
        }
      }
    }
    else {
      foreach ($subfield_names as $subfield_name) {
        $subfield = field_info_field($subfield_name);
        $subfield_access[$subfield_name] = field_access($op, $subfield, 'multifield', NULL, $account);
      }
    }

    // If all of the subfields returned FALSE, then this should return FALSE.
    if (!array_filter($subfield_access)) {
      return FALSE;
    }
  }
}

/**
 * Implements hook_field_validate().
 */
function multifield_field_validate($entity_type, $entity, $field, $instance, $langcode, $items, &$errors) {
  $machine_name = multifield_extract_multifield_machine_name($field);

  foreach ($items as $delta => $item) {
    //$item['parent'] = $entity;
    //$pseudo_entity = _multifield_field_item_to_entity($machine_name, $item);
    //_multifield_field_attach_form_validate('multifield', $pseudo_entity);
    //$items[$delta] = _multifield_field_entity_to_item($pseudo_entity);
  }
}

function _multifield_field_attach_form_validate($entity_type, $entity) {
  // Validate $options since this is a new parameter added after Drupal 7 was
  // released.
  $options = is_array($options) ? $options : array();

  // Extract field values from submitted values.
  //_field_invoke_default('extract_form_values', $entity_type, $entity, $form, $form_state);

  // Perform field_level validation.
  try {
    field_attach_validate($entity_type, $entity, $options);
  }
  catch (FieldValidationException $e) {
    // Pass field-level validation errors back to widgets for accurate error
    // flagging.
    foreach ($e->errors as $field_name => $field_errors) {
      foreach ($field_errors as $langcode => $errors) {
        $field_state = field_form_get_state($form['#parents'], $field_name, $langcode, $form_state);
        $field_state['errors'] = $errors;
        field_form_set_state($form['#parents'], $field_name, $langcode, $form_state, $field_state);
      }
    }
    _field_invoke_default('form_errors', $entity_type, $entity, $form, $form_state, $options);
  }
}

/**
 * Implements hook_field_presave().
 */
function multifield_field_presave($entity_type, $entity, $field, $instance, $langcode, &$items) {
  $machine_name = multifield_extract_multifield_machine_name($field);

  // Gather the original field values from the parent entity if it has them.
  $original_items = !empty($entity->original->{$field['field_name']}[$langcode]) ? $entity->original->{$field['field_name']}[$langcode] : array();

  foreach ($items as $delta => $item) {
    // If an ID is not yet assigned, add one, unless this hook is invoked
    // from the default value widget on the field instance (empty $entity).
    // If this is a new or cloned entity, ensure that the internal ID values
    // are reset.
    if (empty($item['id']) && !empty($entity)) {
      $item['id'] = multifield_get_next_id();
    }
    // If an ID is assigned, but this is a new entity, then ensure that the ID
    // is reassigned a new value.
    // Cannot rely on $entity->is_new being set here because it usually gets
    // added after calling field_attach_presave().
    elseif (!empty($item['id']) && empty($entity->original)) {
      $item['id'] = multifield_get_next_id();
    }

    $item['parent'] = $entity;
    $pseudo_entity = _multifield_field_item_to_entity($machine_name, $item);

    // Add the original item value to the pseudo-entity.
    $pseudo_entity->original = FALSE;
    if (!empty($original_items[$delta])) {
      $original_items[$delta]['parent'] = $entity;
      $pseudo_entity->original = _multifield_field_item_to_entity($machine_name, $original_items[$delta]);
    }

    // Ensure the revision flag is set.
    if (!empty($entity->revision)) {
      $pseudo_entity->revision = TRUE;
    }

    // Run each sub-field through hook_field_presave().
    _multifield_field_invoke('presave', $machine_name, 'multifield', $pseudo_entity, $langcode);

    unset($pseudo_entity->original);
    $items[$delta] = _multifield_field_entity_to_item($pseudo_entity);
  }

  // Invoke hook_field_presave() with the sub-field data attached to a stub of
  // the parent entity.
  $stub_entity = _multifield_create_stub_entity_with_subfield_data($machine_name, $items, $original_items, $entity_type, $entity, $langcode);
  _multifield_field_invoke('presave', $machine_name, $entity_type, $stub_entity, $langcode);

  // Serialize the multifield values into separate columns for saving into the
  // field table.
  array_walk($items, 'multifield_item_serialize', $machine_name);
}

/**
 * Implements hook_field_attach_presave().
 */
function multifield_field_attach_presave($entity_type, $entity) {
  // Run each pseudo-entity through hook_field_attach_presave().
  _multifield_entity_multifields_pseudo_entities_module_invoke($entity_type, $entity, 'field_attach_presave');
}

/**
 * Implements hook_field_attach_submit().
 */
function multifield_field_attach_submit($entity_type, $entity, $form, &$form_state) {
  list(, , $bundle) = entity_extract_ids($entity_type, $entity);
  $multifields = multifield_get_fields();
  $instances = array_intersect_key(field_info_instances($entity_type, $bundle), $multifields);
  foreach (array_keys($instances) as $field_name) {
    $machine_name = $multifields[$field_name];
    if (!empty($entity->{$field_name})) {
      foreach ($entity->{$field_name} as $langcode => &$items) {
        foreach ($items as $delta => &$item) {
          $item['parent'] = $entity;
          $pseudo_entity = _multifield_field_item_to_entity($machine_name, $item);

          // Run each sub-field through hook_field_submit().
          _multifield_field_invoke_default('submit', $machine_name, 'multifield', $pseudo_entity, $langcode, $form, $form_state);

          // Run each pseudo-entity through hook_field_attach_submit().
          foreach (module_implements('field_attach_submit') as $module) {
            $function = $module . '_field_attach_submit';
            $function('multifield', $pseudo_entity, $form, $form_state);
          }

          $item = _multifield_field_entity_to_item($pseudo_entity);
        }
      }
    }
  }
}

/**
 * Implements hook_field_is_empty().
 */
function multifield_field_is_empty($item, $field) {
  $machine_name = multifield_extract_multifield_machine_name($field);
  $subinstances = field_info_instances('multifield', $machine_name);
  foreach ($subinstances as $subfield_name => $subinstance) {
    if (!empty($item[$subfield_name])) {
      $subfield = field_info_field($subfield_name);
      foreach ($item[$subfield_name] as $langcode => $subfield_items) {
        $item[$subfield_name][$langcode] = _field_filter_items($subfield, $subfield_items);
      }
      $item[$subfield_name] = array_filter($item[$subfield_name]);
    }
  }
  return !array_filter(array_intersect_key($item, $subinstances));
}

/**
 * Implements hook_field_insert().
 */
function multifield_field_insert($entity_type, $entity, $field, $instance, $langcode, &$items) {
  $machine_name = multifield_extract_multifield_machine_name($field);

  // Because field default values are loaded after hook_field_presave() is run,
  // manually call it again if a default value was used.
  if (!empty($instance['default_value']) && $items === $instance['default_value']) {
    // @todo Revisit why this is necessary and if it could be simplified or removed.
    multifield_field_presave($entity_type, $entity, $field, $instance, $langcode, $items);
  }

  // Run each sub-field through hook_field_insert().
  foreach ($items as $delta => $item) {
    $item['parent'] = $entity;
    $pseudo_entity = _multifield_field_item_to_entity($machine_name, $item);

    // Run each sub-field through hook_field_insert().
    _multifield_field_invoke_default('insert', $machine_name, 'multifield', $pseudo_entity, $langcode);
    _multifield_field_invoke('insert', $machine_name, 'multifield', $pseudo_entity, $langcode);

    // Run each multifield pseudo-entity through hook_field_storage_pre_insert().
    // @todo This invocation should probably be moved to a multifield_field_storage_pre_insert().
    $skip_fields = array();
    foreach (module_implements('field_storage_pre_insert') as $module) {
      $function = $module . '_field_storage_pre_insert';
      $function('multifield', $pseudo_entity, $skip_fields);
    }

    $items[$delta] = _multifield_field_entity_to_item($pseudo_entity);
  }

  // Invoke hook_field_insert() with the sub-field data attached to a stub of
  // the parent entity.
  $stub_entity = _multifield_create_stub_entity_with_subfield_data($machine_name, $items, array(), $entity_type, $entity, $langcode);
  _multifield_field_invoke('insert', $machine_name, $entity_type, $stub_entity, $langcode);

  // Because this is invoked right prior to field storage writing, we need to
  // re-serialize the field values.
  array_walk($items, 'multifield_item_serialize', $machine_name);

  multifield_update_maximum_id($items);
}

/**
 * Implements hook_field_attach_insert().
 */
function multifield_field_attach_insert($entity_type, $entity) {
  // Run each pseudo-entity through hook_field_attach_insert().
  _multifield_entity_multifields_pseudo_entities_module_invoke($entity_type, $entity, 'field_attach_insert');
}

/**
 * Implements hook_field_update().
 */
function multifield_field_update($entity_type, $entity, $field, $instance, $langcode, &$items) {
  $machine_name = multifield_extract_multifield_machine_name($field);

  // Gather the original field values from the parent entity if it has them.
  $original_items = !empty($entity->original->{$field['field_name']}[$langcode]) ? $entity->original->{$field['field_name']}[$langcode] : array();

  foreach ($items as $delta => $item) {
    $item['parent'] = $entity;
    $pseudo_entity = _multifield_field_item_to_entity($machine_name, $item);

    // Add the original item value to the pseudo-entity.
    $pseudo_entity->original = FALSE;
    if (!empty($original_items[$delta])) {
      $pseudo_entity->original = _multifield_field_item_to_entity($machine_name, $original_items[$delta]);
    }
    // Ensure the revision flag is set.
    if (!empty($entity->revision)) {
      $pseudo_entity->revision = TRUE;
    }

    // Run each sub-field through hook_field_update().
    _multifield_field_invoke('update', $machine_name, 'multifield', $pseudo_entity, $langcode);

    // Run each multifield pseudo-entity through hook_field_storage_pre_update().
    // @todo This invocation should probably be moved to a multifield_field_storage_pre_update().
    $skip_fields = array();
    foreach (module_implements('field_storage_pre_update') as $module) {
      $function = $module . '_field_storage_pre_update';
      $function('multifield', $pseudo_entity, $skip_fields);
    }

    unset($pseudo_entity->original);
    $items[$delta] = _multifield_field_entity_to_item($pseudo_entity);
  }

  // Invoke hook_field_update() with the sub-field data attached to a stub of
  // the parent entity.
  $stub_entity = _multifield_create_stub_entity_with_subfield_data($machine_name, $items, $original_items, $entity_type, $entity, $langcode);
  _multifield_field_invoke('update', $machine_name, $entity_type, $stub_entity, $langcode);

  // Because this is invoked right prior to field storage writing, we need to
  // re-serialize the field values.
  array_walk($items, 'multifield_item_serialize', $machine_name);

  multifield_update_maximum_id($items);
}

/**
 * Implements hook_field_attach_update().
 */
function multifield_field_attach_update($entity_type, $entity) {
  // Run each pseudo-entity through hook_field_attach_delete().
  _multifield_entity_multifields_pseudo_entities_module_invoke($entity_type, $entity, 'field_attach_update');
}

/**
 * Implements hook_field_delete_revision().
 */
function multifield_field_delete_revision($entity_type, $entity, $field, $instance, $langcode, &$items) {
  $machine_name = multifield_extract_multifield_machine_name($field);

  foreach ($items as $delta => $item) {
    $item['parent'] = $entity;
    $pseudo_entity = _multifield_field_item_to_entity($machine_name, $item);

    // Run each sub-field through hook_field_delete_revision().
    _multifield_field_invoke('delete_revision', $machine_name, 'multifield', $pseudo_entity, $langcode);

    //$items[$delta] = _multifield_field_entity_to_item($pseudo_entity);
  }

  // Invoke hook_field_delete() with the sub-field data attached to a stub of
  // the parent entity.
  $stub_entity = _multifield_create_stub_entity_with_subfield_data($machine_name, $items, array(), $entity_type, $entity, $langcode);
  _multifield_field_invoke('delete_revision', $machine_name, $entity_type, $stub_entity, $langcode);
}

/**
 * Implements hook_field_attach_delete_revision().
 */
function multifield_field_attach_delete_revision($entity_type, $entity) {
  // Run each pseudo-entity through hook_field_attach_delete_revision().
  _multifield_entity_multifields_pseudo_entities_module_invoke($entity_type, $entity, 'field_attach_delete_revision');
}

/**
 * Implements hook_field_delete().
 */
function multifield_field_delete($entity_type, $entity, $field, $instance, $langcode, &$items) {
  $machine_name = multifield_extract_multifield_machine_name($field);

  foreach ($items as $delta => $item) {
    $item['parent'] = $entity;
    $pseudo_entity = _multifield_field_item_to_entity($machine_name, $item);

    // Run each sub-field through hook_field_delete().
    _multifield_field_invoke('delete', $machine_name, 'multifield', $pseudo_entity, $langcode);

    //$items[$delta] = _multifield_field_entity_to_item($pseudo_entity);
  }

  // Fetch all the past revisions of this entity and run all the previous
  // multifield values through hook_field_delete_revision().
  $info = entity_get_info($entity_type);
  if (!empty($info['revision table']) && !empty($info['entity keys']['revision'])) {
    $id_key = $info['entity keys']['id'];
    $entity_id = $entity->{$id_key};
    $revision_key = $info['entity keys']['revision'];
    $query = db_select($info['revision table'], 'revision');
    $query->addField('revision', $revision_key);
    $query->condition('revision.' . $id_key, $entity_id);
    $revision_ids = $query->execute()->fetchCol();
    foreach ($revision_ids as $revision_id) {
      if ($revisions = entity_load_multiple($entity_type, array($entity_id), array($revision_key => $revision_id))) {
        $revision = reset($revisions);
        if (!empty($revision->{$field['field_name']})) {
          foreach ($revision->{$field['field_name']} as $revision_langcode => $revision_items) {
            multifield_field_delete_revision($entity_type, $revision, $field, $instance, $langcode, $revision_items);
          }
        }
      }
    }
  }

  // Invoke hook_field_delete() with the sub-field data attached to a stub of
  // the parent entity.
  $stub_entity = _multifield_create_stub_entity_with_subfield_data($machine_name, $items, array(), $entity_type, $entity, $langcode);
  _multifield_field_invoke('delete', $machine_name, $entity_type, $stub_entity, $langcode);

  if ($ids = multifield_items_extract_ids($items)) {
    entity_get_controller('multifield')->resetCache($ids);
  }
}

/**
 * Implements hook_field_attach_update().
 */
function multifield_field_attach_delete($entity_type, $entity) {
  // Run each pseudo-entity through hook_field_attach_delete().
  _multifield_entity_multifields_pseudo_entities_module_invoke($entity_type, $entity, 'field_attach_delete');
}

/**
 * Implements hook_field_widget_info().
 */
function multifield_field_widget_info() {
  $info['multifield_default'] = array(
    'label' => t('Default'),
    'field types' => array_keys(multifield_field_info()),
  );

  return $info;
}

/**
 * Implements hook_field_attach_form().
 */
function multifield_field_attach_form($entity_type, $entity, &$form, &$form_state, $langcode) {
  foreach (field_info_instances($entity_type, $form['#bundle']) as $field_name => $instance) {
    $field = field_info_field($field_name);
    if (multifield_extract_multifield_machine_name($field) && $field['cardinality'] == FIELD_CARDINALITY_UNLIMITED && !empty($field['settings']['hide_blank_items']) && field_access('edit', $field, $entity_type) ) {
      $element_langcode = $form[$field_name]['#language'];
      if ($form[$field_name][$element_langcode]['#max_delta'] > 0) {
        $form[$field_name][$element_langcode]['#max_delta']--;
      }
    }
  }
}

/**
 * Implements hook_field_widget_form().
 */
function multifield_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  $machine_name = multifield_extract_multifield_machine_name($field);
  $item = isset($items[$delta]) ? $items[$delta] : array();
  $entity = $element['#entity'];
  $is_default_value_widget = empty($entity);

  if ($field['cardinality'] == FIELD_CARDINALITY_UNLIMITED && !empty($field['settings']['hide_blank_items'])) {
    $field_state = field_form_get_state($element['#field_parents'], $element['#field_name'], $element['#language'], $form_state);
    if ($delta == $field_state['items_count'] && $delta > 0) {
      // Do not add a blank item. Also see multifield_field_attach_form() for
      // correcting #max_delta.
      return FALSE;
    }
    elseif ($field_state['items_count'] == 0) {
      // We show one item, so also specify that as item count. So when the
      // add button is pressed the item count will be 2 and we show to items.
      $field_state['items_count'] = 1;
    }
    field_form_set_state($element['#field_parents'], $element['#field_name'], $element['#language'], $form_state, $field_state);
  }

  $element['#parents'] = array_merge($element['#field_parents'], array($element['#field_name'], $element['#language'], $element['#delta']));

  // Force the ID of the pseudo entity to be NULL, to prevent modules like
  // entity reference from trying to use it.
  $item['parent'] = $entity;
  $pseudo_entity = _multifield_field_item_to_entity($machine_name, $item);
  $pseudo_entity->id = NULL;

  // Rather than calling field_attach_form() here, we have to limit these
  // sub-field widgets to only one cardinality value. So manually invoke
  // field_default_form() for each one.
  foreach (field_info_instances('multifield', $machine_name) as $subfield_name => $subinstance) {
    $subfield = field_info_field($subfield_name);
    $subfield['cardinality'] = 1;
    // If a subfield is required, but this is not the first delta, or this
    // widget it being used in the default value form for the multifield, then
    // disable the subfield's requirement flag.
    if ($subinstance['required'] && ($delta > 0 || $is_default_value_widget)) {
      $subinstance['required'] = 0;
    }
    $subitems = isset($pseudo_entity->{$subfield_name}[LANGUAGE_NONE]) ? $pseudo_entity->{$subfield_name}[LANGUAGE_NONE] : array();
    $element += field_default_form('multifield', $pseudo_entity, $subfield, $subinstance, LANGUAGE_NONE, $subitems, $element, $form_state, 0);
  }

  // If this multifield itself has a cardinality of one value, and this is not
  // being used for the field default value form, then set the wrapping element
  // to be a fieldset for display/grouping purposes.
  if ($field['cardinality'] == 1 && !empty($element['#entity'])) {
    $element['#type'] = 'fieldset';
  }

  $element['id'] = array(
    '#type' => 'value',
    '#value' => !empty($item['id']) ? $item['id'] : NULL,
  );

  $element['actions'] = array(
    '#type' => 'container',
    //'#attached' => array(
    //  'css' => array(
    //    backdrop_get_path('module', 'multifield') . '/multifield.field.css' => array(),
    //  ),
    //),
    '#pre_render' => array('_multifield_show_only_if_visible_children'),
    '#weight' => 99,
  );
  $element['actions']['remove_button'] = array(
    '#type' => 'submit',
    '#value' => t('Remove'),
    '#name' => implode('_', $element['#parents']) . '_remove_button',
    '#delta' => $delta,
    '#submit' => array('multifield_field_widget_remove_item_submit'),
    '#limit_validation_errors' => array(),
    '#ajax' => array(
      'path' => 'multifield/field-remove-item/ajax',
      'effect' => 'fade',
    ),
    '#attributes' => array(
      'class' => array('remove-button', 'delta-' . $delta),
    ),
    '#access' => $field['cardinality'] == FIELD_CARDINALITY_UNLIMITED && !$is_default_value_widget,
  );

  _multifield_field_attach_form('multifield', $pseudo_entity, $element, $form_state, LANGUAGE_NONE);
  // Restore the internal entity type and bundle properties.
  $element['#entity_type'] = $instance['entity_type'];
  $element['#entity'] = $entity;
  $element['#bundle'] = $instance['bundle'];

  //$form['#validate'][] = 'multifield_field_widget_validate';
  //$form['#multifields'][] = $element['#parents'];

  return $element;
}

function _multifield_show_only_if_visible_children($elements) {
  if (!element_get_visible_children($elements)) {
    $elements['#printed'] = TRUE;
  }
  return $elements;
}

function _multifield_field_attach_form($entity_type, $entity, &$form, &$form_state, $langcode = NULL, $options = array()) {
  // Validate $options since this is a new parameter added after Drupal 7 was
  // released.
  //$options = is_array($options) ? $options : array();

  // Set #parents to 'top-level' by default.
  //$form += array('#parents' => array());

  // If no language is provided use the default site language.
  //$options['language'] = field_valid_language($langcode);
  //$form += (array) _field_invoke_default('form', $entity_type, $entity, $form, $form_state, $options);

  // Add custom weight handling.
  list($id, $vid, $bundle) = entity_extract_ids($entity_type, $entity);
  $form['#pre_render'][] = '_field_extra_fields_pre_render';
  $form['#entity_type'] = $entity_type;
  $form['#entity'] = $entity;
  $form['#bundle'] = $bundle;

  // Let other modules make changes to the form.
  // Avoid module_invoke_all() to let parameters be taken by reference.
  foreach (module_implements('field_attach_form') as $module) {
    $function = $module . '_field_attach_form';
    $function($entity_type, $entity, $form, $form_state, $langcode);
  }
}

function multifield_field_widget_validate($element, &$form_state) {
  //dpm($element, __FUNCTION__);
  //dpm($form_state, __FUNCTION__);
}

/**
 * Page callback to handle AJAX for removing a multifield item.
 *
 * Copied from field_collection_remove_js().
 *
 * This is a direct page callback. The actual job of deleting the item is
 * done in the submit handler for the button, so all we really need to
 * do is process the form and then generate output. We generate this
 * output by doing a replace command on the id of the entire form element.
 */
function multifield_field_widget_remove_item_ajax() {
  require_once BACKDROP_ROOT . '/core/includes/form.inc';

  // backdrop_html_id() very helpfully ensures that all html IDS are unique
  // on a page. Unfortunately what it doesn't realize is that the IDs
  // we are generating are going to replace IDs that already exist, so
  // this actually works against us.
  if (isset($_POST['ajax_html_ids'])) {
    unset($_POST['ajax_html_ids']);
  }

  list($form, $form_state) = ajax_get_form();
  backdrop_process_form($form['#form_id'], $form, $form_state);

  // Get the information on what we're removing.
  $button = $form_state['triggering_element'];
  // Go two levels up in the form, to the whole widget.
  $element = backdrop_array_get_nested_value($form, array_slice($button['#array_parents'], 0, -4));
  // Now send back the proper AJAX command to replace it.
  $return = array(
    '#type' => 'ajax',
    '#commands' => array(
      ajax_command_replace('#' . $element['#id'], backdrop_render($element))
    ),
  );

  // Because we're doing this ourselves, messages aren't automatic. We have
  // to add them.
  $messages = theme('status_messages');
  if ($messages) {
    $return['#commands'][] = ajax_command_prepend('#' . $element['#id'], $messages);
  }

  return $return;
}

/**
 * Submit callback to remove an item from the field UI multiple wrapper.
 *
 * Copied from field_collection_remove_submit()
 *
 * When a remove button is submitted, we need to find the item that it
 * referenced and delete it. Since field UI has the deltas as a straight
 * unbroken array key, we have to renumber everything down. Since we do this
 * we *also* need to move all the deltas around in the $form_state['values']
 * and $form_state['input'] so that user changed values follow. This is a bit
 * of a complicated process.
 */
function multifield_field_widget_remove_item_submit($form, &$form_state) {
  $button = $form_state['triggering_element'];
  $delta = $button['#delta'];

  // Where in the form we'll find the parent element.
  $address = array_slice($button['#array_parents'], 0, -3);

  // Go one level up in the form, to the widgets container.
  $parent_element = backdrop_array_get_nested_value($form, $address);
  $field_name = $parent_element['#field_name'];
  $langcode = $parent_element['#language'];
  $parents = $parent_element['#field_parents'];

  $field_state = field_form_get_state($parents, $field_name, $langcode, $form_state);

  // Go ahead and renumber everything from our delta to the last
  // item down one. This will overwrite the item being removed.
  for ($i = $delta; $i <= $field_state['items_count']; $i++) {
    $old_element_address = array_merge($address, array($i + 1));
    $new_element_address = array_merge($address, array($i));

    $moving_element = backdrop_array_get_nested_value($form, $old_element_address);
    $moving_element_value = backdrop_array_get_nested_value($form_state['values'], $old_element_address);
    $moving_element_input = backdrop_array_get_nested_value($form_state['input'], $old_element_address);

    // Tell the element where it's being moved to.
    $moving_element['#parents'] = $new_element_address;

    // Move the element around.
    form_set_value($moving_element, $moving_element_value, $form_state);
    backdrop_array_set_nested_value($form_state['input'], $moving_element['#parents'], $moving_element_input);
  }

  // Then remove the last item. But we must not go negative.
  if ($field_state['items_count'] > 0) {
    $field_state['items_count']--;
  }

  // Fix the weights. Field UI lets the weights be in a range of
  // (-1 * item_count) to (item_count). This means that when we remove one,
  // the range shrinks; weights outside of that range then get set to
  // the first item in the select by the browser, floating them to the top.
  // We use a brute force method because we lost weights on both ends
  // and if the user has moved things around, we have to cascade because
  // if I have items weight weights 3 and 4, and I change 4 to 3 but leave
  // the 3, the order of the two 3s now is undefined and may not match what
  // the user had selected.
  $input = backdrop_array_get_nested_value($form_state['input'], $address);
  // Sort by weight
  uasort($input, '_field_sort_items_helper');

  // Reweight everything in the correct order.
  $weight = -1 * $field_state['items_count'];
  foreach ($input as $key => $item) {
    if ($item) {
      $input[$key]['_weight'] = $weight++;
    }
  }

  backdrop_array_set_nested_value($form_state['input'], $address, $input);
  field_form_set_state($parents, $field_name, $langcode, $form_state, $field_state);

  $form_state['rebuild'] = TRUE;
}

/**
 * Implements hook_field_formatter_view().
 */
function multifield_field_formatter_info() {
  $info['multifield_default'] = array(
    'label' => t('Default'),
    'field types' => array_keys(multifield_field_info()),
    'settings' => array(
      'view_mode' => 'default',
    ),
  );

  return $info;
}

/**
 * Implements hook_field_formatter_settings().
 */
function multifield_field_formatter_settings_form($field, $instance, $view_mode, $form, &$form_state) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];

  $entity_info = entity_get_info('multifield');
  $options = array();
  foreach ($entity_info['view modes'] as $view_mode_name => $view_mode) {
    $options[$view_mode_name] = $view_mode['label'];
  }

  $element['view_mode'] = array(
    '#type' => 'select',
    '#title' => t('View mode'),
    '#options' => array('default' => t('Default')) + $options,
    '#default_value' => $settings['view_mode'],
    '#required' => TRUE,
  );

  return $element;
}

/**
 * Implements hook_field_formatter_settings_summary().
 */
function multifield_field_formatter_settings_summary($field, $instance, $view_mode) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];

  $entity_info = entity_get_info('multifield');
  $view_mode_label = $settings['view_mode'] == 'default' ? t('Default') : $entity_info['view modes'][$settings['view_mode']]['label'];

  $summary = t('View mode: @view-mode', array('@view-mode' => $view_mode_label));

  return $summary;
}

/**
 * Implements hook_field_prepare_view().
 */
function multifield_field_prepare_view($entity_type, $entities, $field, $instances, $langcode, &$items, $display = NULL) {
  // When this hook is invoked using field_attach_prepare_view(), the view mode
  // being used to render the entities is not passed through here. So we need
  // a hack to retrieve it. When this hook is invoked using field_view_field(),
  // the display settings are passed into the last parameter.
  if (!isset($display)) {
    $backtrace = debug_backtrace();
    foreach ($backtrace as $caller) {
      if ($caller['function'] == 'field_attach_prepare_view') {
        $display = $caller['args'][2];
      }
    }
    if (!isset($display)) {
      throw new Exception("Unable to determine the view mode being used to render the parent entity of the multifield.");
    }
  }

  $machine_name = multifield_extract_multifield_machine_name($field);
  $view_mode_pseudo_entities = array();

  foreach ($entities as $id => $entity) {
    $instance_display = is_array($display) ? $display : field_get_display($instances[$id], $display, $entity);
    if (!$instance_display['type'] !== 'hidden') {
      $view_mode = !empty($display['settings']['view_mode']) ? $display['settings']['view_mode'] : 'default';
      foreach ($items[$id] as $delta => $item) {
        $item['parent'] = $entity;
        $pseudo_entity = _multifield_field_item_to_entity($machine_name, $item);
        $pseudo_entity->delta = $delta;
        $pseudo_entity->parent_id = $id;
        $view_mode_pseudo_entities[$view_mode][$pseudo_entity->id] = $pseudo_entity;
      }
    }
  }

  foreach ($view_mode_pseudo_entities as $view_mode => $pseudo_entities) {
    field_attach_prepare_view('multifield', $pseudo_entities, $view_mode, $langcode);
    foreach ($pseudo_entities as $pseudo_entity) {
      $item = &$items[$pseudo_entity->parent_id][$pseudo_entity->delta];
      unset($pseudo_entity->delta);
      unset($pseudo_entity->parent_id);
      $item = _multifield_field_entity_to_item($pseudo_entity);
      $item['#pseudo_entity'] = $pseudo_entity;
    }
  }
}

/**
 * Implements hook_field_formatter_view().
 */
function multifield_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  $element = array();
  $settings = $display['settings'];

  foreach ($items as $delta => $item) {
    $element[$delta] = field_attach_view('multifield', $item['#pseudo_entity'], $settings['view_mode'], $langcode);
  }

  return $element;
}

function _multifield_create_stub_entity_with_subfield_data($machine_name, array &$items, array $original_items, $parent_entity_type, $parent_entity, $langcode) {
  // Create a stub clone of the parent entity.
  $stub = entity_create_stub_entity($parent_entity_type, entity_extract_ids($parent_entity_type, $parent_entity));
  if (!empty($parent_entity->original)) {
    $stub->original = entity_create_stub_entity($parent_entity_type, entity_extract_ids($parent_entity_type, $parent_entity->original));
  }
  foreach (multifield_type_get_subfields($machine_name) as $subfield_name) {
    $stub->{$subfield_name} = array($langcode => array());
    if (isset($stub->original)) {
      $stub->original->{$subfield_name} = array($langcode => array());
    }
    foreach ($items as $delta => $item) {
      if (isset($item[$subfield_name][LANGUAGE_NONE][0])) {
        $stub->{$subfield_name}[$langcode][$delta] = &$item[$subfield_name][LANGUAGE_NONE][0];
      }
      if (isset($stub->original) && isset($original_items[$delta][$subfield_name][LANGUAGE_NONE][0])) {
        $stub->original->{$subfield_name}[$langcode][$delta] = $original_items[$delta][$subfield_name][LANGUAGE_NONE][0];
      }
    }
    $stub->{$subfield_name}[$langcode] = array_filter($stub->{$subfield_name}[$langcode]);
    if (isset($stub->original)) {
      $stub->original->{$subfield_name}[$langcode] = array_filter($stub->original->{$subfield_name}[$langcode]);
    }
  }

  // Ensure the revision flag is set.
  if (!empty($parent_entity->revision)) {
    $stub->revision = TRUE;
  }

  return $stub;
}

function _multifield_field_invoke($op, $machine_name, $entity_type, $entity, $langcode, &$a = NULL, &$b = NULL, $options = array()) {
  // Merge default options.
  $default_options = array(
    'default' => FALSE,
    'deleted' => FALSE,
    'language' => NULL,
  );
  $options += $default_options;

  // Determine the list of instances to iterate on.
  $instances = _field_invoke_get_instances('multifield', $machine_name, $options);

  foreach ($instances as $instance) {
    $field = field_info_field($instance['field_name']);
    $field_name = $field['field_name'];
    $function = $options['default'] ? 'field_default_' . $op : $field['module'] . '_field_' . $op;
    if (function_exists($function)) {
      if (isset($entity->{$field_name}[$langcode])) {
        $function($entity_type, $entity, $field, $instance, $langcode, $entity->{$field_name}[$langcode], $a, $b);
      }
    }
  }
}

/**
 * Invoke field.module's version of a field hook.
 *
 * This function invokes the field_default_[op]() function.
 * Use _field_invoke() to invoke the field type implementation,
 * hook_field_[op]().
 *
 * @see _field_invoke()
 */
function _multifield_field_invoke_default($op, $machine_name, $entity_type, $entity, $langcode, &$a = NULL, &$b = NULL, $options = array()) {
  $options['default'] = TRUE;
  _multifield_field_invoke($op, $machine_name, $entity_type, $entity, $langcode, $a, $b, $options);
}
