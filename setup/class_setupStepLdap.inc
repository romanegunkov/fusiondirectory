<?php
/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
  Copyright (C) 2007  Fabian Hickert
  Copyright (C) 2011-2016  FusionDirectory

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

class setupStepLdap extends setupStep
{
  var $header_image = 'geticon.php?context=places&icon=network-server&size=48';

  var $connect_id = FALSE;
  var $bind_id    = FALSE;

  private $lastBase       = '';
  private $lastConnection = '';

  static function getAttributesInfo (): array
  {
    return [
      'connection' => [
        'name'      => _('LDAP connection'),
        'attrs'     => [
          new StringAttribute(
            _('Location name'), _('Name of this connexion to show in the LDAP server list'),
            'location', TRUE,
            'default'
          ),
          new StringAttribute(
            _('Connection URI'), _('URI to contact the LDAP server. Usually starts with ldap://'),
            'connection', TRUE,
            'ldap://localhost:389'
          ),
          new BooleanAttribute(
            _('TLS connection'), _('Should TLS be used to connect to this LDAP server?'),
            'tls', FALSE
          ),
          new SelectAttribute(
            _('Base'), _('The LDAP directory base'),
            'base', TRUE
          )
        ]
      ],
      'auth' => [
        'name'      => _('Authentication'),
        'attrs'     => [
          new CompositeAttribute(
            _('DN of the admin account to use for binding to the LDAP. Base is automatically appended.'),
            'admin',
            [
              new StringAttribute(
                '', '',
                'admin_given', TRUE,
                'cn=admin'
              ),
              new DisplayAttribute(
                '', '', 'base_append'
              )
            ],
            '^(.+)(.*)$',
            '%s%s',
            '',
            _('Admin DN')
          ),
          new PasswordAttribute(
            _('Admin password'), _('Password for the admin account to use for binding to the LDAP'),
            'password', TRUE
          ),
        ]
      ],
      'status' => [
        'name'      => _('Status'),
        'attrs'     => [
          new DisplayAttribute(
            _('Current status'), _('Result of last attempt at checking LDAP binding and basic schemas'),
            'status', FALSE
          ),
        ]
      ]
    ];
  }

  function __construct ($parent)
  {
    parent::__construct($parent);
    $this->update_strings();
    $this->attributesAccess['base']->setSubmitForm(TRUE);
    $this->attributesAccess['admin']->setLinearRendering(TRUE);
    $this->attributesAccess['status']->setAllowHTML(TRUE);
    $this->update_base_choices();
    $this->status = $this->get_connection_status();
  }

  function update_strings ()
  {
    $this->s_short_name   = _('LDAP setup');
    $this->s_title        = _('LDAP connection setup');
    $this->s_description  = _('This dialog performs the basic configuration of the LDAP connectivity for FusionDirectory.');
  }

  function update_base_choices ()
  {
    $attr = @LDAP::get_naming_contexts($this->connection);
    unset($attr['count']);
    if (count($attr)) {
      if (!($this->attributesAccess['base'] instanceof SelectAttribute)) {
        $this->attributesInfo['connection']['attrs']['base'] = new SelectAttribute(
          _('Base'), _('The LDAP directory base'),
          'base', TRUE
        );
      }
      $this->attributesAccess['base']->setChoices($attr);
      $this->attributesAccess['admin']->attributes[1]->setValue(','.$this->base);
    } else {
      $this->attributesInfo['connection']['attrs']['base'] = new StringAttribute(
        _('Base'), _('The LDAP directory base'),
        'base', TRUE
      );
    }
    $this->lastConnection = $this->connection;
    $this->lastBase       = $this->base;
  }

  public function update (): bool
  {
    parent::update();
    $this->connection = preg_replace('/\/$/', '', $this->connection);
    if (($this->base != $this->lastBase) || ($this->connection != $this->lastConnection)) {
      $this->parent->disable_steps_from(($this->parent->step_name_to_id(get_class($this))) + 1);
      $this->lastBase       = $this->base;
      if ($this->connection != $this->lastConnection) {
        $this->update_base_choices();
      }
    }

    $this->attributesAccess['admin']->attributes[1]->setValue(','.$this->base);

    $this->status = $this->get_connection_status();
    if ($this->bind_id && !empty($this->admin) && !empty($this->base)) {
      $this->is_completed = TRUE;
      $this->parent->read_ldap_config($this->get_attributes());
    } else {
      $this->is_completed = FALSE;
    }

    return TRUE;
  }

  function get_connection_status ()
  {
    $this->connect_id = FALSE;
    $this->bind_id    = FALSE;

    @ldap_set_option(NULL, LDAP_OPT_DEBUG_LEVEL, 7);
    $this->connect_id = ldap_connect($this->connection);

    if ($this->tls) {
      if (
        @ldap_set_option($this->connect_id, LDAP_OPT_REFERRALS, 0) &&
        @ldap_start_tls($this->connect_id)
        ) {
        $this->bind_id = @ldap_bind($this->connect_id, $this->admin, $this->password);
      }
      @ldap_set_option($this->connect_id, LDAP_OPT_PROTOCOL_VERSION, 3);
    } else {
      @ldap_set_option($this->connect_id, LDAP_OPT_PROTOCOL_VERSION, 3);
      $this->bind_id = @ldap_bind($this->connect_id, $this->admin, $this->password);
    }

    if (!$this->bind_id) {
      if (empty($this->admin)) {
        $str = sprintf(_("Anonymous bind to server '%s' failed!"), $this->connection);
      } else {
        $str = sprintf(_("Bind as user '%s' failed!"), $this->admin, $this->connection);
      }
      $str .= '<input type="submit" name="ldap_retry" value="'._('Retry').'"/>';
      return '<div style="color:red;">'.$str.'</div>';
    } else {
      if (empty($this->admin)) {
        $str = sprintf(_("Anonymous bind to server '%s' succeeded."), $this->connection);
        $str .= '<input type="submit" name="ldap_refresh" value="'._('Refresh').'"/>';
        return '<div style="color:blue;">'.$str.'</div> <div style="color:red;">'._('Please specify user and password!').'</div>';
      } else {
        $str = sprintf(_("Bind as user '%s' to server '%s' succeeded!"), $this->admin, $this->connection);
        $str .= '<input type="submit" name="ldap_refresh" value="'._('Refresh').'"/>';
        return '<div style="color:green;">'.$str.'</div>';
      }
    }
  }

  function check (): array
  {
    $errors = parent::check();
    if (!empty($errors)) {
      $this->update_base_choices();
    } elseif ($this->is_completed) {
      $checked  = check_schema($this->parent->captured_values);
      $errors   = [];
      foreach ($checked as $check) {
        if (!$check['STATUS']) {
          if ($check['IS_MUST_HAVE']) {
            $errors[] = sprintf(_("%s\nSchema \"%s\": %s"), $check['MSG'], $check['SCHEMA_FILE'], $check['INFO']);
          } else {
            $warning = new FusionDirectoryWarning(
              nl2br(htmlescape(sprintf(
                _("%s\nSchema \"%s\": %s"),
                $check['MSG'],
                $check['SCHEMA_FILE'],
                $check['INFO']
              )))
            );
            $warning->display();
          }
        }
      }
      if (!empty($errors)) {
        $this->is_completed = FALSE;
      }
    }
    return $errors;
  }
}
