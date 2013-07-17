BUILD STATUS
------------
Current build status:
[![Build Status](https://travis-ci.org/discoverygarden/islandora_rest.png?branch=7.x)](https://travis-ci.org/discoverygarden/islandora_rest)

CI Server:
http://jenkins.discoverygarden.ca

CONTENTS OF THIS FILE
---------------------

 * [Summary](#summary)
 * [Requirements](#requirements)
 * [Installation](#installation)
 * [Configuration](#configuration)
 * [Documentation](#documentation)
   * [Describe An Existing Object](#describe-an-existing-object)
   * [Create A New Object](#create-a-new-object)
   * [Modify An Existing Objects Properties](#modify-an-existing-objects-properties)
   * [Delete An Existing Object](#delete-an-existing-object)
   * [Search For Objects](#search-for-objects)
   * [List Existing Relationships](#list-existing-relationships)
   * [Add A New Relationship](#add-a-new-relationship)
   * [Remove An Existing Relationship](#remove-an-existing-relationship)
   * [Describe A Existing Datastream](#describe-a-existing-datastream)
   * [Create A New Datastream](#create-a-new-datastream)
   * [Modify An Existing Datastreams Properties And Content](#modify-an-existing-datastreams-properties-and-content)
   * [Delete An Existing Datastream](#delete-an-existing-datastream)
 * [Todo](#todo)

SUMMARY
-------
This module provides a number of REST end points for fetching/manipulating
objects, datastreams, and object relationships from islandora.

The basic structure of the module is such that we define Drupal Menu’s for each
resource type. Currently the resources are __objects__, __datastreams__,
__relationships__, and __solr__. Each Menu defines a callback that generates the
HTTP responses for any HTTP Methods requested of the given resource
__GET__, __PUT__, __POST__, __DELETE__.

This callback will load a file dedicated to the particular resource located in
includes folder and call a function associated with the HTTP method.

So for example if I make a __GET__ request for an __object__, it will auto load
the includes/object.inc file and call a function mapped to the GET method:

```PHP
function islandora_rest_object_get_response(array $parameters) { ... }
```

If it were a __PUT__ request for an __object__ it would be:

```PHP
function islandora_rest_object_put_response(array $parameters) { ... }
```

These functions are responsible for performing any required actions, say
purge an object, etc. As well as generating a response for the client
which will typically be a JSON response.

REQUIREMENTS
------------

  * [Islandora](http://github.com/islandora/islandora)
  * [Islandora SOLR Search](http://github.com/islandora/islandora_solr_search) (Optional)

INSTALLATION
------------

Assumes the following global XACML polices have been removed from:

$FEDORA_HOME/data/fedora-xacml-policies/repository-policies/default

* deny-inactive-or-deleted-objects-or-datastreams-if-not-administrator.xml
* deny-policy-management-if-not-administrator.xml
* deny-purge-datastream-if-active-or-inactive.xml
* deny-purge-object-if-active-or-inactive.xml

This module will still function with those policies in place but the tests this
module defines will fail. Also when installing Islandora don't forget to deploy
the policies it includes!

[Islandora SOLR Search](http://github.com/islandora/islandora_solr_search) is an
optional dependancy that will allow you to perform SOLR searches via this REST
API. Follow the directions it provides if you wish to enable SOLR searches.

CONFIGURATION
-------------

For each of the REST end-points defined below in the documentation section there
exists a corresponding Drupal Permission (with the exception of SOLR which uses
the permissions defined by the Islandora SOLR Search module).


After enabling this module navigate to www.yoursite.com/admin/people/permissions
and enable the features you want to expose. Note that XACML is still enforced on
all REST end-points and access can still be denied regardless of what Drupal
Permissions are enabled.

DOCUMENTATION
-------------

In an effort to simplify the __datastream__ REST end-point multi-part responses
for __GET__ requests were investigated. With the intention of returning both
the content and properties through a single request. While this is possible it
is not well supported by jQuery, there exists a
[plug-in](http://archive.plugins.jquery.com/project/mpAjax), I am unsure if this
plugin works with all browsers. To get around this issue __GET__ requests for a
__datastream__ can return either the content or properties but not both for the
moment.


Also since __PUT__ / __DELETE__ support is lacking in IE6-9, we've provided the
ability to mock __PUT__ / __DELETE__ requests as __POST__ requests by adding an
additional form-data field **method** to inform the server which method was
actually intended.


At the moment multi-part __PUT__ requests such as the one required to modify an
existing __datastream's__ content and properties are **not implemented** you
can mock these __PUT__ requests using aforementioned mechanism. __POST__ and
include an additional form-data field **method** with the value __PUT__.


Adding support for multi-part __PUT__ requests is possible but would require
writing or using and HTTP Request parsing library as __PUT__ is not well
supported in __PHP__.

### Documentation Key
{variable} Required Parameter.

[variable] Optional Parameter no default defined, NULL or empty string likely be
used.

[variable, ’default’] Optional Parameter and it’s default.

### Common Responses
Unless otherwise specified each end-point can return the following responses.

#### Response: 401 Unauthorized
##### No response body.
In general 401 means an anonymous user attempted some action and was denied.
Either the action is not granted to anonymous uses in the Drupal permissions,
or XACML denied the action for the anonymous user.

#### Response: 403 Forbidden
##### No response body.
In general 403 means an authenticated user attempted some action and was denied.
Either the authenticated user does not have a Drupal role that grants him/her
permission to perform that action, or XACML denied the action for that user.

#### Response: 404 Not Found
##### No response body.
In general a 404 will occur, when a user tries to perform an action on a
object or datastream that doesn't not exist. Or XACML is hiding that
object or datastream from the user.

404 Responses can be returned even if the user was **not** determined to have
permission to perform the requested action, as the resource must be first
fetched from fedora before the users permission can be determined.

#### Response: 500 Internal Server Error
##### Content-Type: application/json
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| message       | A detail description of the error.

Any problem can trigger a 500 error, but in general you'll find that this is
typically an error returned by Fedora.

## Describe An Existing Object

#### URL syntax
islandora/rest/v1/object/{pid}

#### HTTP Method
GET

#### Headers
Accept: application/json

#### Get Parameters
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| pid           | Persistent identifier of the object

#### Response: 200 OK
##### Content-Type: application/json
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| pid           | Persistent identifier of the requested object
| label         | The object’s label
| models        | An array of the objects models.
| state         | Object’s state, either “A”, “I”, “D”
| owner         | The objects owner
| created       | Created date of the object, yyyy-MM-ddTHH:mm:ssZ
| modified      | Last modified date of the object, yyyy-MM-ddTHH:mm:ssZ
| datastreams   | Any array of objects each describing a data stream. See GET datastream for more details.

#### Example Response
```JSON
{
 "pid": "islandora:root",
 "label": "Root Object",
 "owner": "fedoraAdmin",
 "models": ["islandora:collectionCModel"],
 "state": "A",
 "created": "2013-05-27T09:53:39.286Z",
 "modified": "2013-06-24T04:20:26.190Z",
 "datastreams": [{
   "dsid": "RELS-EXT",
   "label": "Fedora Object to Object Relationship Metadata.",
   "state": "A",
   "size": 1173,
   "mimetype": "application\/rdf+xml",
   "controlGroup": "X",
   "created": "2013-06-23T07:28:32.787Z",
   "versionable": true,
   "versions": []
 }]
}
```

## Create A New Object

#### URL syntax
islandora/rest/v1/object

#### HTTP Method
POST

#### Headers
Accept: application/json

#### POST Parameters
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| pid           | Persistent identifier of the object, if not given the namespace will be used to create a new one (optional).
| label         | The label of the new object (optional).
| owner         | The owner of the new object, if not given it will be the currently logged in (optional).
| namespace     | Used to create a PID for a new empty object if pid is not given (optional).

#### Response: 201 Created
##### Content-Type: application/json
Returns the same response as a [GET Object](#response-200-ok) request.

## Modify An Existing Object’S Properties

#### URL syntax
islandora/rest/v1/object/{pid}

#### HTTP Method
PUT

#### Headers
Accept: application/json

#### Get Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| pid           | Persistent identifier of the object, if not given the namespace will be used to create a new one.

#### Request Body Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| label         | The new label of the object (optional)
| owner         | The new owner of the object (optional)
| state         | The new state of the object, either “A”, “I”, “D” (optional)

Only given variables will change the object the others will retain their
original values.

#### Response: 200 OK
##### Content-Type: application/json
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| pid           | Persistent identifier of the requested object
| label         | The object’s label
| state         | Object’s state, either “A”, “I”, “D”
| owner         | The objects owner
| modified      | Last modified date of the object, yyyy-MM-ddTHH:mm:ssZ

## Delete An Existing Object

#### URL syntax
islandora/rest/v1/object/{pid}

#### HTTP Method
DELETE

#### Headers
Accept: application/json

##### Get Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| pid           | Persistent identifier of the object.

#### Response: 200 OK
##### No response body.

## List Existing Relationships

#### URL syntax
islandora/rest/v1/object/{pid}/relationship?[predicate][uri][object][literal, false]

#### HTTP Method
GET

#### Headers
Accept: application/json

#### Get Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| pid           | Persistent identifier of the object.
| predicate     | The predicate to limit the results to. (optional)
| uri           | The uri of the predicate, required if predicate is present.
| object        | The object to limit the results to. (optional)
| literal       | True if the object is literal, false otherwise. Defaults to false. (optional)

#### Response: 200 OK
##### Content-Type: application/json
A JSON **array** with each field containing the following values.

| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| predicate     |  object that describes the predicate
| object        |  An object that describes the object.

#### Example Response

```JSON
[{
  "predicate": {
    "value": "hasModel",
    "alias": "fedora-model",
    "namespace": "info:fedora/fedora-system:def/model#"
  },
  "object": {
    "literal": false,
    "value": "islandora:collectionCModel"
  }
}]
```

## Add A New Relationship

#### URL syntax
islandora/rest/v1/object/{pid}/relationship

#### HTTP Method
POST

#### Headers
Accept: application/json

#### Get Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| pid           | Persistent identifier of the object.

#### Post Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| uri           | The predicate URI for the given predicate
| predicate     | The predicate of the relationship.
| object        | Object of the relationship.
| literal       | True if the object of the relationship is a literal, false if it is a URI
| datatype      | If the object is a literal, the datatype of the literal (optional)

#### Response: 201 Created
##### No response body.

## Remove An Existing Relationship

#### URL syntax
islandora/rest/v1/object/{pid}/relationship

#### HTTP Method
DELETE

#### Get Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| pid           | Persistent identifier of the object.

#### Request Body Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| uri           | The uri of the predicate, required if predicate is present.
| predicate     | The predicate to limit the remove to (optional)
| object        | The object to limit the remove to (optional)
| literal       | True if the object is literal, false otherwise. Defaults to false (optional)

#### Response: 200 Ok
##### No response body.

## Describe A Existing Datastream
#### URL syntax
islandora/rest/v1/object/{pid}/datastream/{dsid}?[content, true][version]

#### HTTP Method
GET

#### Headers
Accept: application/json

#### Get Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| pid           | Persistent identifier of the object
| dsid          | Data stream Identifier.
| content       | True to return the datastream’s content, False to return it’s properties.
| version       | The version of the datastream to return, identified by its created date of the datastream in ISO 8601 format yyyy-MM-ddTHH:mm:ssZ (optional) If not given the latest version of the data stream will be returned.

#### Response: 200 OK
##### Content-Type: application/json
If content is false these properties are returned. When requesting a specific
version of the data stream, these values are limited to a subset described here.

| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| dsid          | The datastream's persistent identifier
| label         | The datastream's label
| size          | The datastream's size in bytes
| state         | The datastream’s state, either “A”, “I”, “D”
| mimetype      | The datastream’s mimetype
| controlGroup  | The datastream's control group, either X, M, E, R
| versionable   | A boolean value if the datastream is versionable
| created       | Created date of the datastream, yyyy-MM-ddTHH:mm:ssZ
| versions      | Any array of objects each describing each datastream version not including the latest, contains a subset of the fields described here.

#### Example Response
```JSON
{
  "dsid": "RELS-EXT",
  "label": "Fedora Object to Object Relationship Metadata.",
  "state": "A",
  "size": 1173,
  "mimetype": "application\/rdf+xml",
  "controlGroup": "X",
  "created": "2013-06-23T07:28:32.787Z",
  "versionable": true,
  "versions": [{
    "label": "Old Label.",
    "state": "A",
    "size": "1000",
    "mimetype": "application\/rdf+xml",
    "controlGroup": "X",
    "created": "2013-05-23T06:26:32.787Z"
  }]
}
```
If content is true the datastream’s content is returned, and the appropriate
content-type will be set by the server for the datastream’s content.

## Create A New Datastream

#### URL syntax
islandora/rest/v1/object/{pid}/datastream

#### HTTP Method
POST

#### Headers
Accept: application/json

#### Get Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| pid           | Persistent identifier of the object to add the datastream to.

#### Post Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| dsid          | The new datastream's persistent identifier
| label         | The new datastream's label (optional)
| state         | The new datastream’s state, either “A”, “I”, “D” (optional) Defaults to “A”
| mimetype      | The new datastream’s mime type (optional) if not provided then it is guessed from the uploaded file.
| controlGroup  | The new datastream's control group, either X, M, E, R
| versionable   | A boolean value if the datastream is versionable (optional) Defaults to true
| multipart file as request content | File to use as the datastream’s content

#### Response: 201 Created
##### Content-Type: application/json
Returns the same response as a [GET Datastream](#response-200-ok-5) request.

## Modify An Existing Datastreams Properties And Content

#### URL syntax
islandora/rest/v1/object/{pid}/datastream/{dsid}

#### HTTP Method
PUT

#### Headers
Accept: application/json

#### Get Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| pid           | Persistent identifier of the object to add the datastream to.
| dsid          | The persistent datastream identifier.

#### Request Body Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| label         | The datastream's new label (optional)
| state         | The datastream’s new state, either “A”, “I”, “D” (optional)
| mimetype      | The new datastream’s mime type (optional) if not provided then it is guessed from the uploaded file.
| versionable   | A boolean value if the datastream is versionable (optional) Defaults to true
| multipart file as request content | File to replace existing datastream (for Managed datastreams)

#### Response: 200 Ok
##### Content-Type: application/json
Returns the same response as a [GET Datastream](#response-200-ok-5) request.

## Delete An Existing Datastream

#### URL syntax
islandora/rest/v1/object/{pid}/datastream/{dsid}

#### HTTP Method
DELETE

#### Headers
Accept: application/json

#### Get Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| pid           | Persistent identifier of the object, if not given the namespace will be used to create a new one.
| dsid          | Data stream Identifier.

#### Response: 200 OK
##### No response body.

## Search For Objects
This is a light wrapper for the SOLR own end point. It incorporates XACML alters
to restrict the search results for the given user.

#### URL syntax
islandora/rest/v1/solr/{query}

#### HTTP Method
GET

#### Headers
Accept: application/json

#### Get Variables
| Name          | Description                                                  |
| ------------- | ------------------------------------------------------------ |
| query         | The SOLR query to execute.
| ...           | See the SOLR documentation for the additional parameters it can take.

#### Response: 200 OK
##### Content-Type: application/json
The result will be unmodified server side since all the query types support the
return of JSON directly to the user. So this will more or less just delegate to
those respective end points. See the documentation for SOLR for more info.

TODO
----
- [ ] Update Solution Packs to use datastream ingested/modified hooks rather
      than object ingested hooks.
- [ ] Move DC transform logic out of XML Forms and have it use ingested/modified
      hooks instead.
- [ ] Add support for purging previous versions of datastreams
- [ ] Add checksum support to datastream end-points.
- [ ] Add describe json end-point for the repository.
- [ ] Make PUT requests support multi-part form data, populate $_FILES.
- [ ] Investigate a making a jQuery plugin to ease interaction with REST API?
