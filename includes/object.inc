<?php

/**
 * @file
 * Generate responses for the object rest end point.
 */

/**
 * Responds to the GET request for this resource.
 *
 * @param array $parameters
 *   An associative array containing relevent data for this request.
 *   - resource: The object to generate the response for.
 *
 * @return array
 *   The un-encoded response containing the following fields.
 *   - pid: The object's pid.
 *   - label: The object's label.
 *   - owner: The object's owner.
 *   - models: An array containing the object's models.
 *   - state: The object's state.
 *   - created: The object's created date.
 *   - modified: The object's last modified date.
 *   - datastreams: An array of datastreams.
 *     - dsid: The datastream's dsid.
 *     - label: The datastream's label.
 *     - state: The datastream's state.
 *     - size: The datastream's size in bytes.
 *     - mimetype: The datastream's mimetype.
 *     - controlGroup: The datastream's control group, either X, M, E, R.
 *     - created: The datastream's created date.
 *     - versionable: A boolean value if the datastream is versionable.
 *     - versions: An array containing previous versions of the datastream.
 *       - label: The datastream's label.
 *       - state: The datastream's state.
 *       - size: The datastream's size in bytes.
 *       - mimetype: The datastream's mimetype.
 *       - controlGroup: The datastream's control group, either X, M, E, R.
 *       - created: The datastream's created date.
 */
function islandora_rest_object_get_response(array $parameters) {
  module_load_include('inc', 'islandora_rest', 'includes/datastream');
  $object = $parameters['resource'];
  $datastreams = array();
  foreach ($object as $datastream) {
    $properties = islandora_rest_get_datastream_properties($datastream);
    if ($properties) {
      $datastreams[] = $properties;
    }
  }
  return array(
    'pid' => $object->id,
    'label' => $object->label,
    'owner' => $object->owner,
    'models' => $object->models,
    'state' => $object->state,
    'created' => (string) $object->createdDate,
    'modified' => (string) $object->lastModifiedDate,
    'datastreams' => $datastreams,
  );
}

/**
 * Responds to the PUT request for this resource.
 *
 * @param array $parameters
 *   An associative array containing relevent data for this request.
 *   - resource: The object to generate the response for.
 *   - request: The request parameters
 *     - label: The object's new label.
 *     - owner: The object's new owner.
 *     - state: The object's new state.
 *
 * @return array
 *   The un-encoded response containing the following fields.
 *   - modified: The objects last modified date.
 */
function islandora_rest_object_put_response(array $parameters) {
  $object = $parameters['resource'];
  $request = $parameters['request'];
  if (isset($request['label'])) {
    $object->label = $request['label'];
  }
  if (isset($request['owner'])) {
    $object->owner = $request['owner'];
  }
  if (isset($request['state'])) {
    $object->state = $request['state'];
  }
  return array(
    'pid' => $object->id,
    'label' => $object->label,
    'owner' => $object->owner,
    'state' => $object->state,
    'modified' => (string) $object->lastModifiedDate,
  );
}

/**
 * Responds to the POST request for this resource.
 *
 * Creates a new object, with the given $params.
 *
 * @param array $parameters
 *   An associative array containing relevent data for this request.
 *   - request: The request parameters
 *     - pid: The new object's pid (optional).
 *     - namespace: The new object's namespace (optional).
 *     - label: The new object's label (optional).
 *     - owner: The new object's owner (optional).
 *
 * @return array
 *   Returns the same response as a GET request to the existing object.
 */
function islandora_rest_object_post_response(array $parameters) {
  $request = $parameters['request'];
  $pid = isset($request['namespace']) ? $request['namespace'] : NULL;
  $pid = isset($request['pid']) ? $request['pid'] : $pid;
  $tuque = islandora_get_tuque_connection();
  $object = $tuque->repository->constructObject($pid);
  $object->label = isset($request['label']) ? $request['label'] : $object->label;
  $object->owner = isset($request['owner']) ? $request['owner'] : $object->owner;
  $tuque->repository->ingestObject($object);
  drupal_add_http_header('Status', 201);
  // Mock the Get Response.
  return islandora_rest_object_get_response(array('resource' => $object));
}

/**
 * Responds to the DELETE request for this resource.
 *
 * Delete's the given object.
 *
 * @param array $parameters
 *   An associative array containing relevent data for this request.
 *   - resource: The object to generate the response for.
 */
function islandora_rest_object_delete_response(array $parameters) {
  $object = $parameters['resource'];
  $object->repository->purgeObject($object->id);
}
