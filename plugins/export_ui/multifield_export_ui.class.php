<?php

class multifield_export_ui extends ctools_export_ui {
  function access($op, $item) {
    if (in_array($op, array('import', 'export', 'revert', 'disable', 'enable'))) {
      return FALSE;
    }
    return parent::access($op, $item);
  }
}
