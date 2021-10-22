<?php
/**
 * @file
 * Integration with the Feeds module.
 */

/**
 * Implements hook_feeds_processor_targets_alter().
 */
function multifield_feeds_processor_targets_alter(&$targets, $entity_type, $bundle) {
  foreach (field_info_instances($entity_type, $bundle) as $name => $instance) {
    $info = field_info_field($name);
    if ($info['type'] == 'multifield') {
      foreach ($info['columns'] as $sub_field => $schema_info) {
        $name_label = $instance['label'] . ': ' . drupal_ucfirst(str_replace('_', ' ', $sub_field));
        $targets[$name . ':' . $sub_field] = array(
          'name' => $name_label,
          'callback' => 'multifield_set_target',
          'real_target' => $name,
        );
      }
    }
  }
}

/**
 * Callback for hook_feeds_processor_targets_alter().
 *
 * @param $source
 *   Field mapper source settings.
 * @param $entity
 *   An entity object, for instance a node object.
 * @param $target
 *   A string identifying the target on the node.
 * @param $values
 *   The value to populate the target with.
 */
function multifield_set_target(FeedsSource $source, $entity, $target, array $values, array $mapping) {
  if (empty($values)) {
    return;
  }

  $language = $mapping['language'];

  list($field_name, $sub_field) = explode(':', $target, 2);

  // remove _value from the end of the subfield name.
  $sub_field = str_replace('_value', '', $sub_field);

  $field = isset($entity->$field_name) ? $entity->$field_name : array($language => array());

  foreach ($values as $delta => $value) {
    // Create the complete value for the subfield.
    $sub_field_value = array(
      'value' => $value,
      'format' => NULL,
      'safe_value' => check_plain($value),
    );

    $field[$language][$delta][$sub_field][$language][0] = $sub_field_value;
  }

  $entity->$field_name = $field;
}
