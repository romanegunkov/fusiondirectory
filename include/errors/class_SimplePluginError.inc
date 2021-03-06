<?php
/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
  Copyright (C) 2019-2020  FusionDirectory

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

/*! \class SimplePluginError
    \brief Error returned by any method of SimplePlugin
*/
class SimplePluginError extends FusionDirectoryError
{
  protected $object;
  protected $tab;
  protected $attribute;

  public function __construct ($origin, string $htmlMessage = '', int $code = 0, Throwable $previous = NULL)
  {
    $this->setOrigin($origin);

    parent::__construct($htmlMessage, $code, $previous);
  }

  public function setOrigin ($origin)
  {
    if ($origin instanceof Attribute) {
      $this->attribute  = $origin;
      $this->tab        = $origin->getParent();
      $this->object     = $this->tab->parent;
    } else {
      $this->attribute = NULL;
      if ($origin instanceof SimpleTab) {
        $this->tab    = $origin;
        $this->object = $this->tab->parent;
      } elseif ($origin instanceof simpleTabs) {
        $this->tab    = NULL;
        $this->object = $origin;
      } elseif ($origin !== NULL) {
        trigger_error('Invalid origin of class '.get_class($origin));
      }
    }
    if (!$this->object instanceof simpleTabs) {
      $this->object = NULL;
    }
  }

  public function toArray (): array
  {
    $array = parent::toArray();

    if (isset($this->object->dn)) {
      $array['dn']  = $this->object->dn;
    }

    if (isset($this->tab)) {
      $array['tab'] = get_class($this->tab);
    }

    if (isset($this->attribute)) {
      $array['attribute'] = $this->attribute->getLdapName();
    }

    return $array;
  }

  public function __toString ()
  {
    $msg = '';

    if (isset($this->object->dn)) {
      $msg .= $this->object->dn.' > ';
    }

    if (isset($this->tab) && isset($this->tab->parent->by_name[get_class($this->tab)])) {
      $msg .= $this->tab->parent->by_name[get_class($this->tab)].' > ';
    }

    if (isset($this->attribute)) {
      $label = $this->attribute->getLabel();
      if (empty($label)) {
        $msg .= $this->attribute->getLdapName();
      } else {
        $msg .= $label;
      }
    }

    return $msg.': '.$this->getMessage();
  }

  public function computeMsgDialogParameters (): array
  {
    $html = '';

    $breadcrumbs = [];

    if (isset($this->object->dn)) {
      $breadcrumbs[] = htmlescape($this->object->dn);
    }

    if (isset($this->tab) && isset($this->tab->parent->by_name[get_class($this->tab)])) {
      $breadcrumbs[] = htmlescape($this->tab->parent->by_name[get_class($this->tab)]);
    }

    if (isset($this->attribute)) {
      $label = $this->attribute->getLabel();
      if (empty($label)) {
        $breadcrumbs[] = '<i>'.htmlescape($this->attribute->getLdapName()).'</i>';
      } else {
        $breadcrumbs[] = htmlescape($label);
      }
      $example = $this->attribute->getExample();
    }

    if (!empty($breadcrumbs)) {
      $html .= implode(htmlescape(' > '), $breadcrumbs);
    }

    $html .= '<br/><br/>';

    $html .= $this->htmlMessage;

    /* Stylize example */
    if (!empty($example)) {
      $html .= '<br/><br/><i>'.htmlescape(sprintf(_('Example: %s'), $example)).'</i> ';
    }

    return [_('Error'), $html, ERROR_DIALOG, FusionDirectoryError::formatTrace($this)];
  }

  public static function relocate ($origin, FusionDirectoryError $error)
  {
    if ($error instanceof SimplePluginError) {
      $error->setOrigin($origin);
      return $error;
    } else {
      return new SimplePluginError($origin, $error->getHtmlMessage(), $error->getCode(), $error);
    }
  }
}
