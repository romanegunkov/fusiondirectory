<?php
/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)

  Copyright (C) 2017-2019  FusionDirectory

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

class TabFilterElement extends FilterElement
{
  protected $tabs;

  public function __construct (managementFilter $parent)
  {
    global $config;

    parent::__construct($parent);

    $this->tabs = [];
    foreach ($this->parent->parent->objectTypes as $type) {
      $infos = objects::infos($type);
      foreach ($config->data['TABS'][$infos['tabGroup']] as $plug) {
        $class = $plug['CLASS'];
        if ($class == $infos['mainTab']) {
          continue;
        }
        if (isset($this->tabs[$class])) {
          continue;
        }
        if (class_available($class)) {
          $classInfos = pluglist::pluginInfos($class);
          if (isset($classInfos['plFilterObject'])) {
            $this->tabs[$class] = [
              'filter'  => $classInfos['plFilterObject'],
              'infos'   => $classInfos,
              'checked' => FALSE,
            ];
          }
        }
      }
    }
  }

  public function update ()
  {
    foreach ($this->tabs as $class => &$tab) {
      $tab['checked'] = isset($_POST['filter_tab_'.$class]);
    }
    unset($tab);
  }

  public function render (): string
  {
    if (empty($this->tabs)) {
      return '';
    }
    $inputs = [];
    foreach ($this->tabs as $class => $tab) {
      $inputs['filter_tab_'.$class] = [
        'name'    => $tab['infos']['plShortName'],
        'desc'    => $tab['infos']['plShortName'].' '.$tab['filter'],
        'icon'    => (isset($tab['infos']['plSmallIcon']) ? $tab['infos']['plSmallIcon'] : NULL),
        'checked' => $tab['checked'],
      ];
    }
    $smarty = get_smarty();
    $smarty->assign('NAME',   _('Tabs'));
    $smarty->assign('INPUTS', $inputs);
    return $smarty->fetch(get_template_path('management/filter-element.tpl'));
  }

  public function getFilters (string $type, array &$filters): bool
  {
    foreach ($this->tabs as $tab) {
      if ($tab['checked']) {
        $filters[] = $tab['filter'];
      }
    }
    return FALSE;
  }
}
