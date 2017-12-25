<?php
/**
 * @file
 * Entity classes and controllers for Multifield module.
 */

/**
 * Defines the taxonomy term entity.
 */
class Multifield extends Entity {

  /**
   * The Multifield ID. Only used for internal lookups.
   *
   * @var integer
   */
  public $mfid;

  /**
   * The machine name of the field added to the parent entity.
   *
   * @var string
   */
  public $machine_name;

  /**
   * The label for the multifield.
   *
   * @var integer
   */
  public $label;

  /**
   * (optional) Description of the term.
   *
   * @var string
   */
  public $description;

  /**
   * Whether the entity is safe to be modified.
   *
   * @var boolean
   */
  public $locked;

  /**
   * The machine name of the multifield entity.
   *
   * @var string
   */
  public $bundle;

/***/

  /**
   * (optional) The ID of the parent entity.
   *
   * It might be useful to include the ID of the parent entity.
   *
   * @var array
   */
  //public $parent_id;

  /**
   * (optional) The entity type of the parent entity.
   *
   * It might be useful to include the type of the parent entity.
   *
   * @var array
   */
  //public $parent_type;


  /**
   * Implements EntityInterface::id().
   */
  public function id() {
    // This should maybe be the parent entity's ID?
    return $this->mfid;
  }

  /**
   * Implements EntityInterface::label().
   */
  public function label() {
    return $this->machine_name;
  }

  /**
   * Implements EntityInterface::bundle().
   */
  public function bundle() {
    return $this->bundle;
  }

  /**
   * Implements EntityInterface::entityType().
   */
  public function entityType() {
    return 'multifield';
  }

  /**
   * Implements EntityInterface::uri()
   */
  public function uri() {
    return array(
      // This may not be allowed. Trying anyway.
    );
  }
}

class MultifieldEntityController extends DefaultEntityController {
  public function load($ids = array(), $conditions = array()) {
    if (!empty($conditions)) {
      throw new InvalidArgumentException('MultifieldEntityController does not support entity_load() using conditions.');
    }

    $entities = array();

    // Create a new variable which is either a prepared version of the $ids
    // array for later comparison with the entity cache, or FALSE if no $ids
    // were passed. The $ids array is reduced as items are loaded from cache,
    // and we need to know if it's empty for this reason to avoid querying the
    // database when all requested entities are loaded from cache.
    $passed_ids = !empty($ids) ? array_flip($ids) : FALSE;
    // Try to load entities from the static cache, if the entity type supports
    // static caching.
    if ($this->cache) {
      $entities += $this->cacheGet($ids, $conditions);
      // If any entities were loaded, remove them from the ids still to load.
      if ($passed_ids) {
        $ids = array_keys(array_diff_key($passed_ids, $entities));
      }
    }

    // Load any remaining entities from the database. This is the case if $ids
    // is set to FALSE (so we load all entities), if there are any ids left to
    // load, if loading a revision, or if $conditions was passed without $ids.
    if ($ids === FALSE || $ids) {
      $entities += $this->queryLoad($ids);
    }

    return $entities;
  }

  protected function queryLoad($ids) {
    $multifields = multifield_get_fields();

    foreach (array_keys($multifields) as $field_name) {
      $query = new EntityFieldQuery();
      if ($ids) {
        $query->fieldCondition($field_name, 'id', $ids, 'IN');
      }
      else {
        $query->fieldCondition($field_name, 'id', 0, '>');
      }
      if ($results = $query->execute()) {
        $pseudo_entities = array();
        $field = field_info_field($field_name);

        foreach ($results as $entity_type => $entities) {
          // Simply doing an entity load on the entities with multifield values
          // will cause the cacheSet() from multifield_field_load() to get
          // invoked.
          $entities = entity_load($entity_type, array_keys($entities));
          foreach ($entities as $entity) {
            if ($items = field_get_items($entity_type, $entity, $field_name)) {
              foreach ($items as $item) {
                $pseudo_entities[$item['id']] = _multifield_field_item_to_entity($field['type'], $item);
              }
            }
          }
        }

        $this->cacheSet($pseudo_entities);
      }
    }

    return array_intersect_key($this->entityCache, backdrop_map_assoc($ids, $ids));
  }

  public function cacheSet($entities) {
    $this->entityCache += $entities;
  }
}
