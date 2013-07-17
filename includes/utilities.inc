<?php

/**
 * @file
 * Helper functions.
 */

/**
 * Gets the response for the given resource.
 *
 * @param string $endpoint
 *   The rest end-point to generate the response for.
 * @param array $path_parameters
 *   The GET parameters that were embedded within the path of the URL. They
 *   typically contain the following fields:
 *   - pid: Optional, the PID of the requested resource
 *   - dsid: Optional, the DSID of the requested resource.
 *   - query: Optional, the SOLR query to perform.
 *
 * @return string
 *   A JSON encode string, representing the response for the requested resource.
 */
function islandora_rest_get_response($endpoint, array $path_parameters) {
  module_load_include('inc', 'islandora_rest', "includes/{$endpoint}");
  $path_parameters = array_filter($path_parameters);
  $request_method = islandora_rest_get_request_method();
  list($pid, $dsid) = islandora_rest_get_pid_dsid_from_path_parameters($path_parameters);
  try {
    // Fetch the requested resource. Throws 404, if the resource is not found.
    $resource = islandora_rest_get_requested_resource($pid, $dsid);
    // Check Drupal Permissions. Throws 401, 403, if access is denied.
    islandora_rest_require_access($endpoint, $request_method, $resource);
    // Generate the Response.
    $response_function = "islandora_rest_{$endpoint}_{$request_method}_response";
    if (function_exists($response_function)) {
      $parameters = array(
        'path' => $path_parameters,
        'request' => islandora_rest_get_request_parameters(),
        'resource' => $resource,
      );
      // Can throw 400, 401, 500, etc.
      $response = $response_function($parameters);
    }
  }
  catch (Exception $e) {
    drupal_add_http_header('Status', $e->getCode());
    $response = array('message' => $e->getMessage());
  }
  $encode = is_array($response) || is_object($response);
  if ($encode) {
    // Reponses that don't generate objects or arrays, must set their own
    // content types.
    drupal_add_http_header('Content-Type', 'application/json; utf-8');
    return drupal_json_encode($response);
  }
  return $response;
}

/**
 * Checks to see if the current user has access for the requested resource.
 *
 * @throw Exception If not access is not granted to the user. If the user is not
 * logged in throw a 'Unauthorized' exception, if they are logged in throw a
 * 'Forbidden' exception.
 *
 * @param string $endpoint
 *   The rest end-point to generate the response for.
 * @param array $request_method
 *   The request method (GET/POST/PUT/DELETE).
 * @param object $resource
 *   The requested Fedora object/datastream (optional).
 *
 * @return bool
 *   TRUE if all parameters are present FALSE otherwise.
 */
function islandora_rest_require_access($endpoint, $request_method, $resource = NULL) {
  if (!islandora_rest_accessible($endpoint, $request_method, $resource)) {
    if (user_is_anonymous()) {
      throw new Exception('Unauthorized', 401);
    }
    else {
      throw new Exception('Forbidden', 403);
    }
  }
}

/**
 * Check module permissions etc, to see if the current action is allowed.
 *
 * @param string $endpoint
 *   The rest end-point to generate the response for.
 * @param array $request_method
 *   The request method (GET/POST/PUT/DELETE)
 * @param object $resource
 *   The requested Fedora object/datastream (optional).
 *
 * @return bool
 *   TRUE if the user is allowed to access the rest end-point, FALSE otherwise.
 */
function islandora_rest_accessible($endpoint, $request_method, $resource = FALSE) {
  // SOLR resource is a special exception in that it's permissions are defined
  // by another module.
  if ($endpoint == 'solr') {
    return islandora_rest_solr_access();
  }
  $perm = islandora_rest_resource_access_permission($endpoint, $request_method);
  if (is_subclass_of($resource, 'AbstractObject')) {
    return islandora_object_access($perm, $resource);
  }
  elseif (is_subclass_of($resource, 'AbstractDatastream')) {
    return islandora_datastream_access($perm, $resource);
  }
  return user_access($perm);
}

/**
 * Gets the PID and DSID from the path parameters.
 *
 * The resource paths can optionally include a PID and DSID, so this will
 * return FALSE for those not present.
 *
 * @param array $path_parameters
 *   The GET parameters that were embedded within the path of the URL. They
 *   typically contain the following fields:
 *   - pid: Optional, the PID of the requested resource
 *   - dsid: Optional, the DSID of the requested resource.
 *
 * @return array
 *   Where the first variable is the PID if given in the URL path, and second is
 *   the DSID if given in the URL path, if either is not given it will be FALSE.
 */
function islandora_rest_get_pid_dsid_from_path_parameters(array $path_parameters) {
  $pid = !empty($path_parameters['pid']) ? $path_parameters['pid'] : FALSE;
  $dsid = !empty($path_parameters['dsid']) ? $path_parameters['dsid'] : FALSE;
  return array($pid, $dsid);
}

/**
 * Gets the defined access permission for the given resource and request method.
 *
 * @param string $endpoint
 *   The rest end-point to generate the response for.
 * @param array $request_method
 *   The request method (GET/POST/PUT/DELETE).
 */
function islandora_rest_resource_access_permission($endpoint, $request_method) {
  $perm = strtoupper("PERM_{$endpoint}_{$request_method}");
  return defined($perm) ? constant($perm) : FALSE;
}

/**
 * Access callback for the SOLR end-point.
 *
 * @return bool
 *   TRUE if the users can make SOLR queries, FALSE otherise.
 */
function islandora_rest_solr_access() {
  return module_exists('islandora_solr') && user_access('search islandora solr');
}

/**
 * Get's the intended request method.
 *
 * Some browsers like IE6-9 can't make PUT/DELETE requests, so we allow them to
 * mock them using POST with an addition form-data field 'method'.
 *
 * @return string
 *   The intended request method, either 'GET', 'POST', 'PUT', 'DELETE'.
 */
function islandora_rest_get_request_method() {
  if ($_SERVER['REQUEST_METHOD'] == 'POST' && isset($_POST['method'])) {
    return strtoupper($_POST['method']);
  }
  return $_SERVER['REQUEST_METHOD'];
}

/**
 * Get the request parameters.
 *
 * @return array
 *   An array containing the request parameters.
 */
function islandora_rest_get_request_parameters() {
  switch ($_SERVER['REQUEST_METHOD']) {
    case 'GET':
      return $_GET;

    case 'POST':
      // Can POST Fields or Content Body, prefer Fields.
      if (empty($_POST)) {
        return islandora_rest_get_request_body();
      }
      return $_POST;

    case 'PUT':
    case 'DELETE':
      return islandora_rest_get_request_body();

    default:
      throw new Exception('Method Not Allowed', 405);
  }
}

/**
 * Get the requested resource if one is defined.
 *
 * @throw Exception If the requested resource can not be returned.
 *
 * @param string $pid
 *   The PID of the resource to fetch.
 * @param string $dsid
 *   The DSID of the resource to fetch.
 *
 * @return object
 *   The requested resource, which will be a AbstractObject or
 *   AbstractDatastream. If no resource is requested then NULL is returned.
 */
function islandora_rest_get_requested_resource($pid = FALSE, $dsid = FALSE) {
  if (!$pid) {
    return NULL;
  }
  try {
    // Attempt to get the object and if given the data-stream.
    $tuque = islandora_get_tuque_connection();
    $object = $tuque->repository->getObject(urldecode($pid));
    if (!$dsid) {
      return $object;
    }
    if (!isset($object[$dsid])) {
      throw new Exception('Not Found', 404);
    }
    return $object[$dsid];
  }
  catch (Exception $e) {
    if ($dsid) {
      $message = t('Failed to retrieve the requested datastream: "@dsid" from "@pid"', array(
                   '@pid' => $pid,
                   '@dsid' => $dsid,
                 ));
    }
    else {
      $message = t('Failed to retrieve the requested object: "@pid"', array(
                   '@pid' => $pid));
    }
    throw new Exception($message, $e->getCode(), $e);
  }
}

/**
 * Gets the payload from a PUT/DELETE request.
 *
 * HTTP PUT/DELETE isn't well supported in PHP, so this is our only option.
 *
 * @return array
 *   The JSON decoded payload if defined.
 */
function islandora_rest_get_request_body() {
  $body = &drupal_static(__FUNCTION__);
  if (isset($body)) {
    return $body;
  }
  if (isset($_SERVER['CONTENT_LENGTH']) && $_SERVER['CONTENT_LENGTH'] > 0) {
    $stdin = fopen("php://input", "r");
    $body = fread($stdin, $_SERVER['CONTENT_LENGTH']);
    $content_type = isset($_SERVER["CONTENT_TYPE"]) ? $_SERVER["CONTENT_TYPE"] : '';
    switch ($content_type) {
      case 'application/json':
      default:
        // At the moment we are assume only JSON will be included in the request
        // body. To support multi-part PUT request we would have to implement
        // some request body parser here.
        $body = drupal_json_decode($body);
        break;
    }
  }
  return $body;
}

/**
 * Checks to see if all the required parameters are present for the request.
 *
 * @throw Exception If not all the required parameters are present throw a
 * 'Bad Request' exception.
 *
 * @param array $params
 *   The parameters for the request.
 * @param array $required
 *   The required parameters for the request.
 */
function islandora_rest_require_parameters(array $params, array $required) {
  $missing_params = array_diff($required, array_keys($params));
  if (count($missing_params)) {
    $message = t('Bad Request: Missing required fields @fields', array(
                 '@fields' => implode(',', $missing_params)));
    throw new Exception($message, 400);
  }
}
