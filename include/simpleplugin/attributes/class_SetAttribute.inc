<?php
/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
  Copyright (C) 2012-2016  FusionDirectory

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
*/

/*! \brief This class allow to handle easily a multi-valuated attribute
 *
 */
class SetAttribute extends Attribute
{
  public $attribute;
  protected $valueUnicity     = TRUE;
  protected $editingValue     = FALSE;
  protected $linearRendering  = TRUE;
  protected $size             = 4;

  /*! \brief The constructor of SetAttribute
   *
   *  \param Attribute $attribute The attribute you want to see multi-valuated
   *  \param array $values The default values
   *  \param boolean $valueUnicity Should the value unicity be checked
   */
  function __construct (Attribute $attribute, array $values = [], bool $valueUnicity = TRUE)
  {
    parent::__construct(
      $attribute->getLabel(),     $attribute->getDescription(),
      $attribute->getLdapName(),  $attribute->isRequired(),
      $values
    );
    $this->attribute = $attribute;
    $this->attribute->setRequired(TRUE);
    $this->attribute->setIsSubAttribute(TRUE);
    $this->valueUnicity = $valueUnicity;
  }

  protected function getValueCount ()
  {
    return count($this->getValue());
  }

  function setIsSubAttribute (bool $bool)
  {
    parent::setIsSubAttribute($bool);
    $this->attribute->setIsSubAttribute($this->isSubAttribute);
  }

  function setManagedAttributes (array $dontcare)
  {
    trigger_error('method setManagedAttributes is not supported for SetAttributes');
  }

  function setLinearRendering (bool $bool)
  {
    $this->linearRendering = $bool;
  }

  protected function loadAttrValue (array $attrs)
  {
    if (isset($attrs[$this->getLdapName()]['count'])) {
      $this->value = [];
      for ($i = 0; $i < $attrs[$this->getLdapName()]['count']; $i++) {
        $this->value[] = $attrs[$this->getLdapName()][$i];
      }
      sort($this->value);
    }
  }

  function getAcl (): string
  {
    if ($this->attribute === FALSE) {
      return parent::getAcl();
    }
    return $this->attribute->getAcl();
  }

  function getAclInfo ()
  {
    if ($this->attribute === FALSE) {
      return parent::getAclInfo();
    }
    return $this->attribute->getAclInfo();
  }

  function setAcl (string $acl)
  {
    if ($this->attribute === FALSE) {
      return parent::setAcl($acl);
    }
    $this->attribute->setAcl($acl);
  }

  function addPostValue ($value)
  {
    if ($value === '') {
      return FALSE;
    }
    if ($this->valueUnicity && in_array($value, $this->postValue, TRUE)) {
      return FALSE;
    }
    $this->postValue[] = $value;
    return TRUE;
  }

  function delPostValue ($key)
  {
    unset($this->postValue[$key]);
  }

  function loadPostValue ()
  {
    $this->editingValue = FALSE;
    $id = $this->getHtmlId();
    if ($this->isVisible()) {
      $this->postValue = $this->value;
      if (isset($_POST["add".$id])) {
        if ($this->attribute !== FALSE) {
          $this->attribute->loadPostValue();
          $this->attribute->applyPostValue();
          $this->addPostValue($this->attribute->getValue());
        }
      } elseif (isset($_POST["del".$id]) && isset($_POST["row".$id])) {
        foreach ($_POST["row".$id] as $key) {
          $this->delPostValue($key);
        }
      } elseif ($this->attribute !== FALSE) {
        $this->attribute->loadPostValue();
        $this->attribute->applyPostValue();
        $this->editingValue = $this->attribute->getValue();
      }
    }
  }

  function check ()
  {
    $error = parent::check();
    if (!empty($error) || ($this->attribute === FALSE)) {
      return $error;
    } else {
      $values = $this->getValue();
      if (!is_array($values)) {
        return new SimplePluginCheckError(
          $this,
          SimplePluginCheckError::invalidValue(_('Value is not an array'))
        );
      }
      foreach ($values as $value) {
        $this->attribute->setValue($value);
        $error = $this->attribute->check();
        if (!empty($error)) {
          return $error;
        }
      }
    }
  }

  function renderFormInput (): string
  {
    $display      = $this->renderOnlyFormInput();
    $attr_display = $this->renderAttributeInput(FALSE);
    $buttons      = $this->renderButtons();
    return $this->renderAcl($display).$attr_display.$this->renderAcl($buttons);
  }

  function renderTemplateInput (): string
  {
    $display      = $this->renderOnlyFormInput();
    $attr_display = $this->renderAttributeInput(TRUE);
    $buttons      = $this->renderButtons();
    return $this->renderAcl($display).$attr_display.$this->renderAcl($buttons);
  }

  function renderOnlyFormInput (): string
  {
    if (($this->size < 15) && ($this->size < $this->getValueCount())) {
      $this->size = min(15, $this->getValueCount());
    }
    $id = $this->getHtmlId();
    $smarty = get_smarty();
    $smarty->assign($id.'_values', $this->getDisplayValues());
    $display = '<select multiple="multiple"  name="row'.$id.'[]" id="row'.$id.'" size="'.$this->size.'"'.
                ($this->disabled ? ' disabled="disabled"' : '').
                ' >'."\n";
    $display .= '{html_options options=$'.$id.'_values}';
    $display .= '</select><br/>'."\n";
    return $display;
  }

  function getDisplayValues ()
  {
    if ($this->attribute === FALSE) {
      return $this->getValue();
    }
    $attribute = $this->attribute;
    return array_map(
      function ($value) use ($attribute)
      {
        return $attribute->displayValue($value);
      },
      $this->getValue()
    );
  }

  function handleEditingValue ()
  {
    if ($this->editingValue === FALSE) {
      $this->attribute->resetToDefault();
    } else {
      $this->attribute->setValue($this->editingValue);
    }
  }

  function renderAttributeInput ($template = FALSE)
  {
    if ($this->attribute === FALSE) {
      return;
    }
    $this->handleEditingValue();
    if ($this->valueUnicity && ($this->attribute instanceOf SelectAttribute)) {
      $this->attribute->setHiddenChoices($this->getValue());
    }
    if ($template) {
      return $this->attribute->renderTemplateInput();
    } else {
      return $this->attribute->renderFormInput();
    }
  }

  function renderAttribute (array &$attributes, bool $readOnly, bool $readable, bool $writable)
  {
    if ($this->attribute === FALSE) {
      return parent::renderAttribute($attributes, $readOnly, $readable, $writable);
    }
    if ($this->visible) {
      $this->attribute->setDisabled($this->disabled);
      if ($this->linearRendering || $readOnly) {
        parent::renderAttribute($attributes, $readOnly, $readable, $writable);
      } else {
        if ($this->valueUnicity && ($this->attribute instanceOf SelectAttribute)) {
          $this->attribute->setHiddenChoices($this->getValue());
        }
        $attributes[$this->getLdapName()] = [
          'htmlid'        => $this->getForHtmlId(),
          'label'         => '{literal}'.htmlescape($this->getLabel()).'{/literal}',
          'description'   => ($this->isRequired() ? sprintf(_("%s (required)"), $this->getDescription()) : $this->getDescription()),
          'input'         => $this->renderAcl($this->renderOnlyFormInput()),
          'subattribute'  => $this->isSubAttribute,
          'required'      => $this->isRequired(),
          'readable'      => $readable,
          'writable'      => $writable,
        ];
        $this->handleEditingValue();
        $this->attribute->renderAttribute($attributes, $readOnly, $readable, $writable);
        $attributes[$this->getLdapName().'_buttons'] = [
          'htmlid'        => 'add'.$this->getHtmlId(),
          'label'         => '',
          'description'   => '',
          'input'         => $this->renderAcl($this->renderButtons()),
          'subattribute'  => TRUE,
          'required'      => FALSE,
          'readable'      => $readable,
          'writable'      => $writable,
        ];
      }
    }
  }

  function serializeAttribute (array &$attributes, bool $form = TRUE)
  {
    parent::serializeAttribute($attributes, $form);
    if ($this->attribute === FALSE) {
      return;
    }
    if ($form) {
      return;
    } else {
      $subattributes = [];
      $this->attribute->setDisabled($this->disabled);
      $this->attribute->serializeAttribute($subattributes, $form);
      $attributes[$this->getLdapName()]['attributes']       = $subattributes;
      $attributes[$this->getLdapName()]['attributes_order'] = array_keys($subattributes);
    }
  }

  function getForHtmlId (): string
  {
    // Label should point to the attribute
    if (is_object($this->attribute)) {
      return $this->attribute->getForHtmlId();
    } else {
      return '';
    }
  }

  function renderButtons ()
  {
    $id = $this->getHtmlId();
    $buttons  = $this->renderInputField(
      'submit', 'add'.$id,
      [
        'value'           => msgPool::addButton(FALSE),
        'formnovalidate'  => 'formnovalidate',
        'class'           => 'subattribute',
      ]
    );
    $buttons .= $this->renderInputField(
      'submit', 'del'.$id,
      [
        'value'           => msgPool::delButton(FALSE),
        'formnovalidate'  => 'formnovalidate',
        'class'           => 'subattribute',
      ]
    );
    return $buttons;
  }

  function computeLdapValue ()
  {
    $this->sortValues();
    return array_values($this->getValue());
  }

  protected function sortValues ()
  {
    sort($this->value);
  }

  public function htmlIds (): array
  {
    $id = $this->getHtmlId();
    return array_merge(['add'.$id,'del'.$id,'row'.$id], $this->attribute->htmlIds());
  }

  /*! \brief Set the parent plugin for this attribute
   *
   *  \param simplePlugin &$plugin The parent plugin
   */
  function setParent (&$plugin)
  {
    parent::setParent($plugin);
    if ($this->attribute !== FALSE) {
      $this->attribute->setParent($plugin);
    }
  }

  function getArrayValues ()
  {
    $result = [];
    foreach ($this->getValue() as $value) {
      $this->attribute->setValue($value);
      $row = [];
      foreach ($this->attribute->getArrayValue() as $val) {
        $row[] = $val;
      }
      $result[] = $row;
    }
    return $result;
  }

  function foreignKeyUpdate ($oldvalue, $newvalue, array $source)
  {
    foreach ($this->value as $key => &$value) {
      if (($source['FIELD'] == 'dn') && ($source['MODE'] == 'move')) {
        if ($newvalue === NULL) {
          if (preg_match('/'.preg_quote($oldvalue, '/').'$/', $value)) {
            unset($this->value[$key]);
          }
        } else {
          $value = preg_replace('/'.preg_quote($oldvalue, '/').'$/', $newvalue, $value);
        }
      } elseif ($value == $oldvalue) {
        if ($newvalue === NULL) {
          unset($this->value[$key]);
        } elseif ($source['MODE'] == 'copy') {
          $this->value[] = $newvalue;
        } elseif ($source['MODE'] == 'move') {
          $value = $newvalue;
        }
      }
    }
    unset($value);
  }

  function foreignKeyCheck ($value, array $source): bool
  {
    return in_array($value, $this->getValue());
  }

  function setSize ($size)
  {
    $this->size = $size;
  }

  function checkValue ($value)
  {
    if (!is_array($value)) {
      throw new InvalidValueException(sprintf(_('SetAttribute "%s" was set to a non-compatible value'), $this->getLabel()));
    }
  }
}

/*!
 * \brief Multivalued attribute displayed as a columned table. May allow ordering and/or editing
 */
class OrderedArrayAttribute extends SetAttribute
{
  protected $order;
  protected $edit_enabled;
  protected $headers  = FALSE;

  /*! \brief The constructor of OrderedArrayAttribute
   *
   *  \param Attribute $attribute The composite attribute you want to see multi-valuated
   *  \param boolean $order Can the values be ordered
   *  \param array $values The default values
   *  \param boolean $edit_enabled Can the values be edited
   */
  function __construct ($attribute, $order = TRUE, $values = [], $edit_enabled = FALSE)
  {
    parent::__construct($attribute, $values);
    $this->order        = $order;
    $this->edit_enabled = $edit_enabled;
  }

  function setHeaders ($h)
  {
    $this->headers = $h;
  }

  function readValue ($value)
  {
    if ($this->order) {
      return preg_split('/:/', $value, 2);
    } else {
      return $value;
    }
  }

  function writeValue ($key, $value)
  {
    if ($this->order) {
      return $key.":".$value;
    } else {
      return $value;
    }
  }

  function computeLdapValue ()
  {
    $this->sortValues();
    $ldapValue = [];
    foreach ($this->getValue() as $key => $value) {
      $ldapValue[] = $this->writeValue($key, $value);
    }
    return $ldapValue;
  }

  protected function sortValues ()
  {
    if (!$this->order) {
      sort($this->value);
    }
  }

  protected function loadAttrValue (array $attrs)
  {
    if (isset($attrs[$this->getLdapName()]["count"])) {
      $this->value = [];
      for ($i = 0; $i < $attrs[$this->getLdapName()]["count"]; $i++) {
        $value = $this->readValue($attrs[$this->getLdapName()][$i]);
        if (is_array($value)) {
          $this->value[$value[0]] = $value[1];
        } else {
          $this->value[] = $value;
        }
      }
    }
    if ($this->order) {
      ksort($this->value);
      $this->reIndexValues();
    } else {
      $this->sortValues();
    }
  }

  function renderOnlyFormInput (): string
  {
    if (($this->size < 15) && ($this->size < $this->getValueCount())) {
      $this->size = min(15, $this->getValueCount());
    }
    $id = $this->getHtmlId();
    $div = new divSelectBox('rows'.$id);
    $smarty = get_smarty();
    $height = ($this->size * 26) + 6;
    if ($this->headers) {
      $height += 29;
    }
    $div->setHeight($height);
    $div->setHeaders($this->headers);
    foreach ($this->getValue() as $key => $value) {
      $fields = [];
      foreach ($this->getAttributeArrayValue($key, $value) as $field) {
        if (is_array($field)) {
          $fields[] = $field;
        } else {
          $fields[] = ['string' => $field];
        }
      }
      if (empty($fields)) {
        continue;
      }

      list ($img, $nbicons) = $this->genRowIcons($key, $value);
      if ($nbicons > 0) {
        $fields[] = ['html' => $img, 'attach' => 'style="border:0px;width:'.($nbicons * 20).'px;"'];
      }
      $div->addEntry($fields);
    }
    $smarty->assign("div_$id", $div->drawList());
    return '{$div_'.$id.'}'."\n";
  }

  protected function genRowIcons ($key, $value)
  {
    $id = $this->getHtmlId();

    $img      = '';
    $nbicons  = 1;

    if ($this->order) {
      $nbicons += 2;
      if ($key != 0) {
        $img .= $this->renderInputField(
          'image', $id.'_up_'.$key,
          [
            'src'             => 'geticon.php?context=actions&icon=view-sort-descending&size=16',
            'title'           => _('Sort up'),
            'alt'             => _('Sort up'),
            'class'           => 'center',
            'formnovalidate'  => 'formnovalidate',
          ],
          FALSE
        );
      } else {
        $img .= '<img src="images/empty.png" alt="" style="width:16px;"/>';
      }
      if (($key + 1) < $this->getValueCount()) {
        $img .= $this->renderInputField(
          'image', $id.'_down_'.$key,
          [
            'src'             => 'geticon.php?context=actions&icon=view-sort-ascending&size=16',
            'title'           => _('Sort down'),
            'alt'             => _('Sort down'),
            'class'           => 'center',
            'formnovalidate'  => 'formnovalidate',
          ],
          FALSE
        );
      } else {
        $img .= '<img src="images/empty.png" alt="" style="width:16px;"/>';
      }
    }
    if ($this->edit_enabled) {
      $nbicons++;
      $img .= $this->renderInputField(
        'image', $id.'_edit_'.$key,
        [
          'src'             => 'geticon.php?context=actions&icon=document-edit&size=16',
          'title'           => _('Edit'),
          'alt'             => _('Edit'),
          'class'           => 'center',
          'formnovalidate'  => 'formnovalidate',
        ],
        FALSE
      );
    }
    $img .= $this->renderInputField(
      'image', $id.'_del_'.$key,
      [
        'src'             => 'geticon.php?context=actions&icon=edit-delete&size=16',
        'title'           => _('Delete'),
        'alt'             => _('Delete'),
        'class'           => 'center',
        'formnovalidate'  => 'formnovalidate',
      ],
      FALSE
    );

    return  [$img, $nbicons];
  }

  protected function getAttributeArrayValue ($key, $value)
  {
    $this->attribute->setValue($value);
    return $this->attribute->getArrayValue();
  }

  protected function reIndexValues ()
  {
    $this->value = array_values($this->value);
  }

  function loadPostValue ()
  {
    $this->editingValue = FALSE;
    if ($this->isVisible()) {
      $this->postValue = $this->value;
      $id = $this->getHtmlId();
      foreach (array_keys($_POST) as $name) {
        if ($this->handlePostValueActions($id, $name)) {
          break;
        }
      }
      $this->handleAddAndEditValue();
    }
  }

  protected function handlePostValueActions ($id, $postValue)
  {
    if ($this->order) {
      if (preg_match('/^'.$id.'_up_/', $postValue)) {
        $key = preg_replace('/^'.$id.'_up_/', '', $postValue);
        $key = (int)preg_replace('/_[xy]$/', '', $key);

        $tmp                        = $this->postValue[$key];
        $this->postValue[$key]      = $this->postValue[$key - 1];
        $this->postValue[$key - 1]  = $tmp;
        return TRUE;
      }
      if (preg_match('/^'.$id.'_down_/', $postValue)) {
        $key = preg_replace('/^'.$id.'_down_/', '', $postValue);
        $key = (int)preg_replace('/_[xy]$/', '', $key);

        $tmp                        = $this->postValue[$key];
        $this->postValue[$key]      = $this->postValue[$key + 1];
        $this->postValue[$key + 1]  = $tmp;
        return TRUE;
      }
    }
    if ($this->edit_enabled && preg_match('/^'.$id.'_edit_/', $postValue)) {
      $key = preg_replace('/^'.$id.'_edit_/', '', $postValue);
      $key = preg_replace('/_[xy]$/', '', $key);
      $this->handleEdit($key);
      return TRUE;
    }
    if (preg_match('/^'.$id.'_del_/', $postValue)) {
      $key = preg_replace('/^'.$id.'_del_/', '', $postValue);
      $key = preg_replace('/_[xy]$/', '', $key);
      $this->delPostValue($key);
      return TRUE;
    }
    return FALSE;
  }

  protected function handleAddAndEditValue ()
  {
    $id = $this->getHtmlId();
    if ($this->attribute === FALSE) {
      return;
    }
    if (isset($_POST["add$id"])) {
      $this->attribute->loadPostValue();
      $this->attribute->applyPostValue();
      if ($error = $this->attribute->check()) {
        if (is_string($error)) {
          $error = new SimplePluginCheckError($this, $error);
        }
        $error->display();
      } else {
        $this->addPostValue($this->attribute->getValue());
      }
    } elseif ($this->editingValue === FALSE) {
      $this->attribute->loadPostValue();
      $this->attribute->applyPostValue();
      $this->editingValue = $this->attribute->getValue();
    }
  }

  protected function handleEdit ($key)
  {
    $this->editingValue = $this->value[$key];
    $this->delPostValue($key);
    $this->plugin->focusedField = $this->getHtmlId();
  }

  function applyPostValue ()
  {
    parent::applyPostValue();
    if ($this->order) {
      $this->reIndexValues();
    }
  }

  public function htmlIds (): array
  {
    $id   = $this->getHtmlId();
    $ids  = ['add'.$id];
    if ($this->attribute !== FALSE) {
      $ids  = array_merge($ids, $this->attribute->htmlIds());
    }
    $nb_values = $this->getValueCount();
    for ($i = 0; $i < $nb_values; ++$i) {
      if ($this->order) {
        if ($i > 0) {
          $ids[] = $id.'_up_'.$i;
        }
        if (($i + 1) < $nb_values) {
          $ids[] = $id.'_down_'.$i;
        }
      }
      $ids[] = $id.'_del_'.$i;
    }
    return $ids;
  }

  function renderButtons ()
  {
    $id = $this->getHtmlId();
    $buttons = $this->renderInputField(
      'submit', 'add'.$id,
      [
        'value'           => msgPool::addButton(FALSE),
        'formnovalidate'  => 'formnovalidate',
        'class'           => 'subattribute',
      ]
    );
    return $buttons;
  }
}

/*!
 * \brief An OrderedArrayAttribute which stores its values in LDAP subnodes
 */
class SubNodesAttribute extends OrderedArrayAttribute
{
  protected $objectClass;
  protected $objectClasses;

  function __construct ($label, $description, $ldapName, $objectClass, $attributes, $order = FALSE, $values = [], $edit_enabled = FALSE, $acl = "")
  {
    $attributes_keys = [];
    foreach ($attributes as $attribute) {
      $attributes_keys[$attribute->getLdapName()] = $attribute;
      $attributes_keys[$attribute->getLdapName()]->htmlid_prefix = $ldapName.'_';
    }
    $composite = new CompositeAttribute(
      $description, $ldapName,
      $attributes_keys,
      FALSE, FALSE,
      $acl, $label
    );
    parent::__construct($composite, $order, $values, $edit_enabled);
    if (is_array($objectClass)) {
      $this->objectClass    = $objectClass[0];
      $this->objectClasses  = $objectClass;
    } else {
      $this->objectClass    = $objectClass;
      $this->objectClasses  = [$objectClass];
    }
  }

  protected function loadAttrValue (array $attrs)
  {
    global $config;
    /* Should we take dn from attrs or plugin? */
    if (isset($attrs['dn'])) {
      $ldap = $config->get_ldap_link();
      $ldap->cd($attrs['dn']);
      $ldap->search('objectClass='.$this->objectClass, ['*'], 'one');
      $this->value = [];
      while ($subattrs = $ldap->fetch(TRUE)) {
        $this->attribute->resetToDefault();
        foreach ($this->attribute->attributes as &$attribute) {
          $attribute->loadAttrValue($subattrs);
        }
        unset($attribute);
        $this->value[] = $this->attribute->getValue();
      }
      sort($this->value);
    }
  }

  /* Not saving anything into base node */
  function fillLdapValue (array &$attrs)
  {
  }

  /* Special LDAP treatment that this attribute does after plugin ldap save */
  function postLdapSave ($ldap)
  {
    /* First delete all old nodes */
    $ldap->cd($this->plugin->dn);
    $ldap->search('objectClass='.$this->objectClass, ['dn'], 'one');
    $delete = [];
    while ($attrs = $ldap->fetch()) {
      $delete[] = $attrs['dn'];
    }
    foreach ($delete as $dn) {
      $ldap->rmdir($dn);
    }
    /* Then add our values */
    foreach ($this->value as $val) {
      $attrs = ['objectClass' => $this->objectClasses];
      $this->attribute->setValue($val);
      foreach ($this->attribute->attributes as &$attribute) {
        $attribute->fillLdapValue($attrs);
      }
      unset($attribute);
      $dn = $this->compute_attribute_dn();
      $ldap->cd($dn);
      foreach (array_keys($attrs) as $index) {
        if (is_array($attrs[$index]) && (count($attrs[$index]) == 0)) {
          unset($attrs[$index]);
        }
      }
      $ldap->add($attrs);
      if (!$ldap->success()) {
        $error = new SimplePluginLdapError($this, $dn, LDAP_ADD, $ldap->get_error(), $ldap->get_errno());
        $error->display();
      }
    }
  }

  function compute_attribute_dn ()
  {
    /* Later we might want to be able to choose which attribute to use in the dn */
    reset($this->attribute->attributes);
    $firstAttribute = key($this->attribute->attributes);
    return $firstAttribute.'='.$this->attribute->attributes[$firstAttribute]->computeLdapValue().','.$this->plugin->dn;
  }
}
