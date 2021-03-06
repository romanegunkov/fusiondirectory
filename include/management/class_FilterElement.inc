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

/*!
 * \brief This class handles an element from the management filter box
 */
class FilterElement
{
  protected $parent;

  public function __construct (managementFilter $parent)
  {
    $this->parent = $parent;
  }

  /* Reads POST */
  public function update ()
  {
  }

  public function render (): string
  {
    return '';
  }

  /* Fills LDAP filters for the given type. Returns TRUE if type should be skipped altogether. */
  public function getFilters (string $type, array &$filters): bool
  {
    return FALSE;
  }
}
