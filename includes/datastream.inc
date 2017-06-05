<?php

/**
 * @file
 * Generate responses for the datastream rest end point.
 */

/**
 * Responds to the GET request for this resource.
 *
 * @param array $parameters
 *   - resource: The datastream to generate the response for.
 *   - request: The request parameters.
 *     - version: The version of the datastream to return, identified by its
 *       created date of the datastream in ISO 8601 format yyyy-MM-ddTHH:mm:ssZ
 *     - content: TRUE to return the content of the datastream FALSE to return
 *       the datastream's properties. (optional) Defaults to TRUE.
 *
 * @return array
 *   The un-encoded response containing the following fields.
 *   - dsid: The datastream's dsid.
 *   - label: The datastream's label.
 *   - state: The datastream's state.
 *   - size: The datastream's size in bytes.
 *   - mimeType: The datastream's MIME Type.
 *   - controlGroup: The datastream's control group, either X, M, E, R.
 *   - created: The datastream's created date.
 *   - versionable: A boolean value if the datastream is versionable.
 *   - versions: A array containing previous version of the datastream.
 *     - label: The datastream's label.
 *     - state: The datastream's state.
 *     - size: The datastream's size in bytes.
 *     - mimeType: The datastream's MIME Type.
 *     - controlGroup: The datastream's control group, either X, M, E, R.
 *     - created: The datastream's created date.
 */
function islandora_rest_datastream_get_response(array $parameters) {
  $datastream = $parameters['resource'];
  $request = $parameters['request'];
  $requested_version = isset($request['version']) ? $request['version'] : FALSE;
  $requested_content = isset($request['content']) ? filter_var($request['content'], FILTER_VALIDATE_BOOLEAN) : TRUE;
  if ($requested_version) {
    $datastream = islandora_rest_get_datastream_version($datastream, $requested_version);
    if (!$datastream) {
      $message = t('Datastream version: @version not found', array('@version' => $request['version']));
      throw new Exception($message, 404);
    }
  }
  if ($requested_content) {
    module_load_include('inc', 'islandora', 'includes/datastream');
    // This function exits.
    islandora_download_datastream($datastream);
  }
  return $requested_version ?
    islandora_rest_get_datastream_version_properties($datastream) :
    islandora_rest_get_datastream_properties($datastream);
}

/**
 * Responds to the PUT request for this resource.
 *
 * Modifies and existing datastream, with the given $params.
 *
 * @param array $parameters
 *   - resource: The datastream to generate the response for.
 *   - request: The request parameters.
 *     - label: The datastream's label.
 *     - state: The datastream's state.
 *     - mimeType: The datastream's MIME Type.
 *     - checksumType: The datastream's checksum type.
 *     - versionable: Turn versioning on/off.
 */
function islandora_rest_datastream_put_response(array $parameters) {
  $request = $parameters['request'];
  $datastream = $parameters['resource'];
  $file = array_shift($_FILES);
  if (isset($file['error']) && $file['error'] === UPLOAD_ERR_OK) {
    $mime_detect = new MimeDetect();
    $datastream->setContentFromFile($file['tmp_name']);
    $datastream->mimetype = isset($request['mimeType']) ?
      $request['mimeType'] :
      $mime_detect->getMimetype($file['name']);
  }
  if (isset($request['label'])) {
    $datastream->label = $request['label'];
  }
  if (isset($request['state'])) {
    $datastream->state = $request['state'];
  }
  if (isset($request['versionable'])) {
    $datastream->versionable = (bool) $request['versionable'];
  }
  if (isset($request['mimeType'])) {
    $datastream->mimetype = $request['mimeType'];
  }
  if (isset($request['checksumType'])) {
    $datastream->checksumType = $request['checksumType'];
  }
  // Mock the Get Response.
  return islandora_rest_datastream_get_response(array(
      'resource' => $datastream,
      'request' => array(
        'content' => FALSE,
      )));
}

/**
 * Responds to the POST request for this resource.
 *
 * Creates a new datastream, with the given $params.
 *
 * @param array $parameters
 *   - resource: The datastream to generate the response for.
 *   - request: The request parameters.
 *     - dsid: The new datastream's dsid.
 *     - controlGroup: The new datastream's control group either 'X', 'M', 'E',
 *       'R'. Defaults to 'M' (optional).
 *     - label: The new datastream's label, the file name will be used if none
 *       is provided (optional).
 *     - state: The new datastream's state (optional).
 *     - mimeType: The new datastream's MIME Type, the MIME Type of the uploaded
 *       file will be used if not provided (optional).
 *     - checksumType: The new datastream's checksum type (optional).
 *     - versionable: If the datastream will be versionable, defaults to TRUE
 *       (optional).
 *
 * @return array
 *   Returns the same response as a GET request to the existing datastream.
 */
function islandora_rest_datastream_post_response(array $parameters) {
  $object = $parameters['resource'];
  $request = isset($parameters['request']) ? $parameters['request'] : array();
  $required = array('dsid');
  islandora_rest_require_parameters($request, $required);
  $file = array_shift($_FILES);
  if (!isset($file['error']) || $file['error'] !== UPLOAD_ERR_OK) {
    throw new Exception(t('Bad Request: File failed to upload'), 400);
  }
  $mime_detect = new MimeDetect();
  $control_group = isset($request['controlGroup']) ? $request['controlGroup'] : 'M';
  $datastream = $object->constructDatastream($request['dsid'], $control_group);
  $datastream->state = isset($request['state']) ? $request['state'] : 'A';
  $datastream->label = isset($request['label']) ? $request['label'] : $file['name'];
  $datastream->versionable = isset($request['versionable']) ? (bool) $request['versionable'] : TRUE;
  $datastream->mimetype = isset($request['mimeType']) ? $request['mimeType'] : $mime_detect->getMimetype($file['name']);
  $datastream->checksumType = isset($request['checksumType']) ? $request['checksumType'] : variable_get('islandora_checksum_checksum_type', 'DISABLED');
  $datastream->setContentFromFile($file['tmp_name']);
  // XXX: The return of FALSE will be deprecated in 7.x-1.10, remove this after.
  try {
    if (!$object->ingestDatastream($datastream)) {
      throw new Exception('Conflict: Datastream already exists', 409);
    }
  }
  catch (DatastreamExistsException $e) {
    throw new Exception('Conflict: Datastream already exists', 409);
  }
  drupal_add_http_header('Status', 201);
  // Mock the Get Response.
  return islandora_rest_datastream_get_response(array(
      'resource' => $datastream,
      'request' => array(
        'content' => FALSE,
      )));
}

/**
 * Responds to the DELETE request for this resource.
 *
 * Delete's the given datastream.
 *
 * @param array $parameters
 *   - resource: The datastream to generate the response for.
 */
function islandora_rest_datastream_delete_response(array $parameters) {
  $datastream = $parameters['resource'];
  $datastream->parent->purgeDatastream($datastream->id);
}

/**
 * Helper function used to fetch the given version of a datastream.
 *
 * @param string $created_date
 *   The created date of the datastream in ISO 8601 format: yyyy-MM-ddTHH:mm:ssZ
 *
 * @return AbstractDatastream
 *   The datastream version if found, otherwise FALSE is returned.
 */
function islandora_rest_get_datastream_version(AbstractDatastream $datastream, $created_date) {
  foreach ($datastream as $version) {
    if ($created_date == (string) $version->createdDate) {
      return $version;
    }
  }
  return FALSE;
}

/**
 * Returns the relevent properties of the given datastream.
 *
 * @param AbstractDatastream $datastream
 *   The datastream to fetch the relevent properties from.
 *
 * @return array
 *   The given datastream properties, including the properties of each version
 *   of the datastream.
 */
function islandora_rest_get_datastream_properties(AbstractDatastream $datastream) {
  try {
    // For lack of a better method, we just test if we can access the content
    // of the datastream, if we can't XACML is blocking access so we don't
    // return it as availible.
    $datastream->content;
  }
  catch (Exception $e) {
    // Ignore the Exception.
    return FALSE;
  }
  $versions = array();
  foreach ($datastream as $index => $version) {
    if ($index === 0) {
      continue;
    }
    $versions[] = islandora_rest_get_datastream_version_properties($version);
  }
  return array(
    'dsid' => $datastream->id,
    'label' => $datastream->label,
    'state' => $datastream->state,
    'size' => (int) $datastream->size,
    'mimeType' => $datastream->mimetype,
    'controlGroup' => $datastream->controlGroup,
    'created' => (string) $datastream->createdDate,
    'versionable' => (bool) $datastream->versionable,
    'checksum' => $datastream->checksum,
    'checksumType' => $datastream->checksumType,
    'versions' => $versions,
  );
}

/**
 * Returns the relevent properties of the given datastream version.
 *
 * @param AbstractDatastream $datastream
 *   The datastream version to fetch the relevent properties from.
 *
 * @return array
 *   The given datastream version's properties.
 */
function islandora_rest_get_datastream_version_properties(AbstractDatastream $datastream) {
  return array(
    'label' => $datastream->label,
    'state' => $datastream->state,
    'size' => (int) $datastream->size,
    'mimeType' => $datastream->mimetype,
    'controlGroup' => $datastream->controlGroup,
    'created' => (string) $datastream->createdDate,
    'checksum' => $datastream->checksum,
    'checksumType' => $datastream->checksumType,
  );
}
