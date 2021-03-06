<?php
/*
  This code is part of FusionDirectory (http://www.fusiondirectory.org/)
  Copyright (C) 2017-2018  FusionDirectory

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
 * \brief Login via POST
 */
class LoginPost extends LoginMethod
{
  /*! \brief List of second factor methods, may be dynamic later */
  static $secondFactorMethods = ['SecondFactorWebAuthn','SecondFactorTotp','SecondFactorRecoveryCode'];

  /*! \brief Displayed name */
  static function getLabel ()
  {
    return _('HTML form');
  }

  static function init ()
  {
    parent::init();

    /* Init second factor methods if needed */
    foreach (static::$secondFactorMethods as $secondFactorMethod) {
      if (!class_available($secondFactorMethod)) {
        continue;
      }
      $secondFactorMethod::init();
    }
  }

  /*! \brief All login steps in the right order for standard POST login */
  static function loginProcess ()
  {
    global $smarty, $config, $message;

    static::init();

    $smarty->assign('focusfield', 'username');

    if (($_SERVER['REQUEST_METHOD'] == 'POST') && isset($_POST['login']) && isset($_POST['username']) && isset($_POST['password'])) {
      static::$username = $_POST['username'];
      static::$password = $_POST['password'];

      $success = static::runSteps([
        'validateUserInput',
        'checkForLockingBranch',
        'ldapLoginUser',
        'loginAndCheckExpired',
        'runSchemaCheck',
      ]);

      if ($success) {
        /* If needed redirect to second factor page */
        $secondFactor = FALSE;
        foreach (static::$secondFactorMethods as $secondFactorMethod) {
          if (!class_available($secondFactorMethod)) {
            continue;
          }
          $secondFactor = ($secondFactorMethod::hasSecondFactor() || $secondFactor);
        }
        if ($secondFactor) {
          static::redirectSecondFactorPage();
        }

        /* Everything went well, redirect to main.php */
        static::redirect();
      }
    }

    static::displayLogin();
  }

  /*! \brief Redirect to the second factor page */
  static protected function redirectSecondFactorPage ()
  {
    session::un_set('connected');
    header('Location: secondfactor.php');
    exit;
  }

  /*! \brief Display the login page and exit() */
  static protected function displayLogin ()
  {
    global $smarty,$message,$config,$ssl,$error_collector,$error_collector_mailto;

    $lang = session::get('lang');

    error_reporting(E_ALL | E_STRICT);

    /* Fill template with required values */
    $username = '';
    if (isset($_POST['username'])) {
      $username = trim($_POST['username']);
    }
    $smarty->assign('date',       gmdate('D, d M Y H:i:s'));
    $smarty->assign('username',   $username);
    $smarty->assign('copynotice', copynotice());
    $smarty->append('css_files',  get_template_path('login.css'));
    $smarty->assign('title',      _('Sign in'));

    /* Some error to display? */
    if (!isset($message)) {
      $message = '';
    }
    $smarty->assign('message', $message);

    /* Display SSL mode warning? */
    if (($ssl != '') && ($config->get_cfg_value('warnSSL') == 'TRUE')) {
      $smarty->assign('ssl', sprintf(htmlescape(_('Warning: %sSession is not encrypted!%s')), '<a href="'.$ssl.'">', '</a>'));
    } else {
      $smarty->assign('ssl', '');
    }

    if (!$config->check_session_lifetime()) {
      $smarty->assign('lifetime', _('Warning: The session lifetime configured in your fusiondirectory.conf will be overridden by php.ini settings.'));
    } else {
      $smarty->assign('lifetime', '');
    }

    /* Generate server list */
    $servers = [];
    if (isset($_POST['server'])) {
      $selected = $_POST['server'];
    } else {
      $selected = $config->data['MAIN']['DEFAULT'];
    }
    foreach ($config->data['LOCATIONS'] as $key => $ignored) {
      $servers[$key] = $key;
    }
    $smarty->assign('server_options', $servers);
    $smarty->assign('server_id',      $selected);

    /* show login screen */
    $smarty->assign('PHPSESSID', session_id());
    if ($error_collector != '') {
      $smarty->assign('php_errors', preg_replace('/%BUGBODY%/', $error_collector_mailto, $error_collector).'</div>');
    } else {
      $smarty->assign('php_errors', '');
    }
    $smarty->assign('msg_dialogs',  msg_dialog::get_dialogs());
    $smarty->assign('usePrototype', 'false');
    $smarty->assign('date',         date('l, dS F Y H:i:s O'));
    $smarty->assign('lang',         preg_replace('/_.*$/', '', $lang));
    $smarty->assign('rtl',          Language::isRTL($lang));

    $smarty->display(get_template_path('headers.tpl'));
    $smarty->assign('version', FD_VERSION);

    $smarty->display(get_template_path('login.tpl'));
    exit();
  }

  /*! \brief Display the second factor page and exit() */
  static function displaySecondFactorPage ()
  {
    global $smarty,$message,$config,$ssl,$error_collector,$error_collector_mailto;

    $lang = session::get('lang');

    error_reporting(E_ALL | E_STRICT);

    /* Fill template with required values */
    $username = '';
    if (isset($_POST['username'])) {
      $username = trim($_POST['username']);
    }
    $smarty->assign('date',       gmdate('D, d M Y H:i:s'));
    $smarty->assign('username',   $username);
    $smarty->assign('copynotice', copynotice());
    $smarty->append('css_files',  get_template_path('login.css'));
    $smarty->assign('title',      _('Second factor'));

    /* Some error to display? */
    if (!isset($message)) {
      $message = '';
    }
    $smarty->assign('message', $message);

    /* show login screen */
    $smarty->assign('PHPSESSID', session_id());
    if ($error_collector != '') {
      $smarty->assign('php_errors', preg_replace('/%BUGBODY%/', $error_collector_mailto, $error_collector).'</div>');
    } else {
      $smarty->assign('php_errors', '');
    }
    $smarty->assign('msg_dialogs',  msg_dialog::get_dialogs());
    $smarty->assign('usePrototype', 'false');
    $smarty->assign('date',         date('l, dS F Y H:i:s O'));
    $smarty->assign('lang',         preg_replace('/_.*$/', '', $lang));
    $smarty->assign('rtl',          Language::isRTL($lang));
    $smarty->assign('CSRFtoken',    CSRFProtection::getToken());

    $methodOutputs = [];

    /* Run second factor methods */
    foreach (static::$secondFactorMethods as $secondFactorMethod) {
      if (!class_available($secondFactorMethod)) {
        continue;
      }
      $methodOutput = $secondFactorMethod::execute();
      if ($methodOutput !== NULL) {
        $methodOutputs[$secondFactorMethod] = $methodOutput;
      }
    }

    $smarty->assign('methodOutputs', $methodOutputs);

    $smarty->display(get_template_path('headers.tpl'));
    $smarty->assign('version', FD_VERSION);

    $smarty->display(get_template_path('secondfactor.tpl'));
    exit();
  }
}
