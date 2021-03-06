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

/*! \brief This class allow to handle easily an File LDAP attribute
 *
 */
class FileAttribute extends Attribute
{
  protected $binary = TRUE;

  function loadPostValue ()
  {
    $this->postValue = $this->value;
    if (!empty($_FILES[$this->getHtmlId()]['name']) && $this->isVisible()) {
      if ($_FILES[$this->getHtmlId()]['size'] <= 0) {
        $error = new SimplePluginError(
          $this,
          htmlescape(sprintf(
            _('Cannot read uploaded file: %s'),
            _('file is empty')
          ))
        );
        $error->display();
      } elseif (!file_exists($_FILES[$this->getHtmlId()]['tmp_name'])) {
        // Is there a tmp file, which we can use ?
        $error = new SimplePluginError(
          $this,
          htmlescape(sprintf(
            _('Cannot read uploaded file: %s'),
            _('file not found')
          ))
        );
        $error->display();
      } elseif (!$handle = @fopen($_FILES[$this->getHtmlId()]['tmp_name'], 'r')) {
        // Can we open the tmp file, for reading
        $error = new SimplePluginError(
          $this,
          htmlescape(sprintf(
            _('Cannot read uploaded file: %s'),
            _('file not readable')
          ))
        );
        $error->display();
      } else {
        // Everything just fine :)

        // Reading content
        $this->readFile($handle);
      }
      // so that we only handle the file once
      $_FILES[$this->getHtmlId()]['name'] = "";
    }
  }

  /*! \brief This function read the file from the given handle and then closes it
   *
   *  \param filehandle $handle The handle on the opened uploaded file
   */
  function readFile ($handle)
  {
    $postValue = fread($handle, 1024);
    while (!feof($handle)) {
      $postValue .= fread($handle, 1024);
    }
    $this->setPostValue($postValue);
    @fclose($handle);
  }

  function renderFormInput (): string
  {
    $id = $this->getHtmlId();
    $display = $this->renderInputField('file', $id);
    return $this->renderAcl($display);
  }

  function displayValue ($value): string
  {
    return sprintf(_('%s (%d bytes)'), $this->getLabel(), mb_strlen($value, '8bit'));
  }

  /*! \brief Serialize this attribute for RPC requests
   *
   * \param array &$attributes the attributes array
   * \param boolean $form
   */
  function serializeAttribute (array &$attributes, bool $form = TRUE)
  {
    if (!$form || $this->visible) {
      parent::serializeAttribute($attributes, $form);

      if ($this->binary) {
        $attributes[$this->getLdapName()]['binary']   = TRUE;
      }
    }
  }

  /*! \brief Serialize value for RPC requests
   *
   *  \param mixed $value the value
   */
  function serializeValue ($value = NULL)
  {
    if ($value === NULL) {
      $value = $this->getValue();
    }
    if ($this->binary) {
      return base64_encode($value);
    } else {
      return $value;
    }
  }

  /*! \brief Apply value from RPC requests
   *
   *  \param mixed $value the value
   */
  function deserializeValue ($value)
  {
    if ($this->disabled) {
      return new SimplePluginError(
        $this,
        htmlescape(sprintf(_('Attribute %s is disabled, its value could not be set'), $this->getLdapName()))
      );
    }
    if ($this->binary) {
      $data = base64_decode($value, TRUE);
      if ($data === FALSE) {
        return new SimplePluginError(
          $this,
          htmlescape(_('Invalid base64 data'))
        );
      }
      $this->setValue($data);
    } else {
      $this->setValue($value);
    }
  }
}

/*!
 * \brief FileAttribue with a download button
 */
class FileDownloadAttribute extends FileAttribute
{
  protected $extension;
  protected $upload;
  protected $download = TRUE;

  function __construct ($label, $description, $ldapName, $required = FALSE, $extension = '', $upload = FALSE, $defaultValue = "", $acl = "")
  {
    parent::__construct($label, $description, $ldapName, $required, $defaultValue, $acl);
    $this->extension  = $extension;
    $this->upload     = $upload;
    $this->binary     = ($extension != '.txt');
  }

  function computeFilename ()
  {
    return $this->getLdapName().$this->extension;
  }

  function loadPostValue ()
  {
    if ($this->isVisible()) {
      $this->postValue = $this->value;
      if ($this->download) {
        foreach (array_keys($_POST) as $name) {
          if (preg_match('/^download'.$this->getHtmlId().'/', $name)) {
            session::set('binary', $this->value);
            session::set('binarytype', 'octet-stream');
            session::set('binaryfile', $this->computeFilename());
            header('location: getbin.php');
            exit();
          }
        }
      }
      if ($this->upload && isset($_POST['upload'.$this->getHtmlId()])) {
        parent::loadPostValue();
      }
    }
  }

  function renderFormInput (): string
  {
    $id = $this->getHtmlId();
    $display = '';
    if ($this->upload) {
      $display  .= $this->renderInputField('file', $id);
      $display  .= $this->renderInputField('submit', 'upload'.$id, ['value' => _('Upload'), 'formnovalidate' => 'formnovalidate']);
    }
    if ($this->download) {
      $display  .= $this->renderInputField(
        'image', 'download'.$id,
        [
          'title' => _('Download'),
          'alt'   => _('Download'),
          'class' => 'center',
          'src'   => 'geticon.php?context=actions&icon=document-save&size=16',
        ]
      );
    }
    return $this->renderAcl($display);
  }

  public function htmlIds (): array
  {
    $id   = $this->getHtmlId();
    $ids  = [];
    if ($this->download) {
      $ids[] = 'download'.$id;
    }
    if ($this->upload) {
      $ids[] = $id;
      $ids[] = 'upload'.$id;
    }
    return $ids;
  }

  function renderAttribute (array &$attributes, bool $readOnly, bool $readable, bool $writable)
  {
    if ($this->upload === FALSE) {
      parent::renderAttribute($attributes, FALSE, $readable, $writable);
    } else {
      parent::renderAttribute($attributes, $readOnly, $readable, $writable);
    }
  }
}

/*!
 * \brief A FileDownloadAttribute which displays a text area to edit the value
 */
class FileTextAreaAttribute extends FileDownloadAttribute
{
  /* Default values are not the same that for FileDownloadAttribute */
  function __construct ($label, $description, $ldapName, $required = FALSE, $extension = '.txt', $upload = TRUE, $download = TRUE, $defaultValue = '', $acl = '')
  {
    parent::__construct(
      $label, $description, $ldapName, $required,
      $extension, $upload, $defaultValue, $acl
    );
    $this->download = $download;
  }

  /*! \brief Update this attributes postValue depending of the $_POST values
   */
  function loadPostValue ()
  {
    if ($this->isVisible()) {
      $this->postValue = $this->value;
      if ($this->download) {
        foreach (array_keys($_POST) as $name) {
          if (preg_match('/^download'.$this->getHtmlId().'/', $name)) {
            session::set('binary', $this->value);
            session::set('binarytype', 'octet-stream');
            session::set('binaryfile', $this->computeFilename());
            header('location: getbin.php');
            exit();
          }
        }
      }
      if ($this->upload) {
        if (isset($_POST['upload'.$this->getHtmlId()])) {
          parent::loadPostValue();
        } else {
          $id = $this->getHtmlId().'_text';
          if (isset($_POST[$id])) {
            $this->setPostValue($_POST[$id]);
          }
        }
      }
    }
  }

  function fixPostValue ($value)
  {
    /* Replace CRLF by LF, to avoid non-ASCII chars */
    return str_replace(["\r\n", "\r"], "\n", $value);
  }

  function renderFormInput (): string
  {
    $id = $this->getHtmlId();
    $display  = '<textarea name="'.$id.'_text" id="'.$id.'_text"'.
                ($this->disabled ? 'disabled="disabled"' : '').'>'.
                '{literal}'.htmlescape($this->getValue()).'{/literal}</textarea><br/>';
    return $this->renderAcl($display).parent::renderFormInput();
  }

  public function htmlIds (): array
  {
    $id     = $this->getHtmlId();
    $ids    = parent::htmlIds();
    $ids[]  = $id.'_text';
    return $ids;
  }
}

/*!
 * \brief Attribute for storing an image
 */
class ImageAttribute extends FileAttribute
{
  protected $width;
  protected $height;
  protected $format;
  protected $forceSize;
  protected $placeholder;

  protected $imagickException;

  function __construct ($label, $description, $ldapName, $required = FALSE, $width = 48, $height = 48, $format = 'png', $forceSize = FALSE, $defaultValue = "", $acl = "")
  {
    parent::__construct($label, $description, $ldapName, $required, $defaultValue, $acl);
    $this->width      = $width;
    $this->height     = $height;
    $this->format     = $format;
    $this->forceSize  = $forceSize;
  }

  function setPlaceholder ($placeholder)
  {
    $this->placeholder = $placeholder;
  }

  /*! \brief Update this attributes postValue depending of the $_POST values
   */
  function loadPostValue ()
  {
    $this->postValue = $this->value;
    $id = $this->getHtmlId();
    if (!$this->disabled && $this->isVisible()) {
      foreach (array_keys($_POST) as $name) {
        if (!$this->isRequired() && preg_match('/^'.$id.'_remove_/', $name)) {
          $this->setPostValue('');
          break;
        }
        if (preg_match('/^'.$id.'_upload_/', $name)) {
          parent::loadPostValue();
          break;
        }
      }
    }
  }

  function setValue ($value)
  {
    $this->imagickException = NULL;
    if ($value == '') {
      $this->value = '';
      return;
    }
    if (class_exists('Imagick')) {
      try {
        $im     = new Imagick();
        $modify = FALSE;
        $im->readImageBlob($value);

        $size = $im->getImageGeometry();

        if (
            ($size['width'] > 0 && $size['height'] > 0) &&
            (
              ($size['width'] < $this->width && $size['height'] < $this->height) ||
              ($size['width'] > $this->width) ||
              ($size['height'] > $this->height)
            )
          ) {
          $modify = TRUE;
          $im->resizeImage($this->width, $this->height, Imagick::FILTER_GAUSSIAN, 1, !$this->forceSize);
        }

        if ($modify || !preg_match('/^'.$this->format.'$/i', $im->getImageFormat())) {
          if ($this->format == 'jpeg') {
            $im->setImageCompression(Imagick::COMPRESSION_JPEG);
            $im->setImageCompressionQuality(90);
          }
          $im->setImageFormat($this->format);

          /* Save attribute */
          $this->value = $im->getImageBlob();
        } else {
          $this->value = $value;
        }
      } catch (ImagickException $e) {
        /* Store the exception to return it in deserializeValue() */
        $this->imagickException = new SimplePluginError(
          $this,
          SimplePluginCheckError::invalidValue($e->getMessage()),
          0,
          $e
        );
        $this->imagickException->display();
      }
    } else {
      $error = new SimplePluginError(
        $this,
        htmlescape(_('Cannot save user picture, FusionDirectory requires the PHP module "imagick" to be installed!'))
      );
      $error->display();
    }
  }

  /*! \brief Apply value from RPC requests
   *
   *  \param mixed $value the value
   */
  function deserializeValue ($value)
  {
    $error = parent::deserializeValue($value);
    if (!empty($error)) {
      return $error;
    }
    if ($this->imagickException !== NULL) {
      return $this->imagickException;
    }
  }

  function renderFormInput (): string
  {
    $this->setValue($this->inputValue($this->getValue()));
    $id = $this->getHtmlId();
    // Just to be sure the image is not cached
    $key      = $this->getLdapName().random_int(0, 10000);
    $display  = '<img id="'.$id.'_img"'.
                ($this->disabled ? 'disabled="disabled"' : '').
                ' src="getbin.php?key='.$key.'"'.
                ' style="border:1px solid black;"'.
                ' alt="'.htmlescape($this->getDescription()).'"'.
                ' title="'.htmlescape($this->getDescription()).'"'.
                ' /><br/>';
    $display  .= $this->renderInputField('file', $id);
    $display .= $this->renderInputField(
      'image', $id.'_upload',
      [
        'class' => 'center',
        'src'   => 'geticon.php?context=actions&icon=upload&size=16',
        'title' => _('Upload'),
        'alt'   => _('Upload')
      ]
    );
    if (!$this->isRequired()) {
      $display .= $this->renderInputField(
        'image', $id.'_remove',
        [
          'class' => 'center',
          'src'   => 'geticon.php?context=actions&icon=remove&size=16',
          'title' => _('Remove'),
          'alt'   => _('Remove')
        ]
      );
    }
    if (($this->getValue() == '') && ($this->placeholder != '')) {
      session::set('binary'.$key, $this->placeholder);
    } else {
      session::set('binary'.$key, $this->getValue());
    }
    session::set('binarytype', 'image/'.$this->format);
    return $this->renderAcl($display);
  }

  public function htmlIds (): array
  {
    $id = $this->getHtmlId();
    return [$id.'_img',$id,'upload'.$id];
  }

  /*! \brief Fill LDAP value in the attrs array
   */
  function fillLdapValue (array &$attrs)
  {
    if ($this->isInLdap()) {
      $value = $this->computeLdapValue();
      if ($value !== '') {
        if ($this->isTemplate()) {
          /* Add %% to provide template from parsing binary string */
          $value = '%%'.$value;
        }
        $attrs[$this->getLdapName()] = $value;
      } else {
        $attrs[$this->getLdapName()] = [];
      }
    }
  }

  function inputValue ($value)
  {
    /* Remove %% that might be there in case of templating */
    return preg_replace('/^%%/', '', $value);
  }
}
