<?php

/**
 * @file
 * Generate responses for the solr rest end point.
 */

/**
 * Responds to a SOLR GET request.
 *
 * @param array $parameters
 *   An associative array containing relevent data for this request.
 *   - path: The GET parameters from the URL path.
 *     - query: The SOLR query.
 *   - request: The request parameters passed onto SOLR.
 *     - See the SOLR documentation for more information.
 *
 * @return array
 *   The un-encoded response containing the generated SOLR results.
 */
function islandora_rest_solr_get_response(array $parameters) {
  module_load_include('inc', 'islandora_solr', 'includes/utilities');

  // XXX: Replace '~slsh~' in query with '/' to get around path splitting.
  $query = islandora_solr_restore_slashes($parameters['path']['query']);
  $request = $parameters['request'];

  $path_parts = parse_url(variable_get('islandora_solr_url', 'localhost:8080/solr'));
  $solr = new Apache_Solr_Service($path_parts['host'], $path_parts['port'], $path_parts['path'] . '/');
  $solr->setCreateDocuments(0);

  // Grab XACML restrictions if the module exists. Note because of how PHP
  // handles parsing of $_GET parameters with the same name, multiple query
  // parameters need to be logically chained together or passed in array form.
  if (module_exists('islandora_xacml_api')) {
    $obj = new stdClass();
    islandora_xacml_api_islandora_solr_query($obj);
    $xacml_fq = reset($obj->solrParams['fq']);
    if (isset($request['fq'])) {
      if (is_array($request['fq'])) {
        $request['fq'][] = $xacml_fq;
      }
      else {
        $request['fq'] = array(
          $xacml_fq,
          $request['fq'],
        );
      }
    }
    else {
      $request['fq'] = $xacml_fq;
    }
  }
  // Query is executed. Errors are caught and sent through to the response.
  $offset = isset($request['start']) ? (int) $request['start'] : 0;
  $limit = isset($request['rows']) ? (int) $request['rows'] : variable_get('islandora_solr_num_of_results', 20);
  try {
    $results = $solr->search($query, $offset, $limit, $request, 'GET');
  }
  catch (Exception $e) {
    watchdog('islandora_rest', 'Invalid Solr request: @message', array('@message' => $e->getMessage()), WATCHDOG_WARNING);
    $message = $e->getCode() == 400 ? t('Invalid Solr request.') : t('An error occurred and the request could not be processed.');
    throw new Exception($message, $e->getCode());
  }
  return json_decode($results->getRawResponse(), TRUE);
}
