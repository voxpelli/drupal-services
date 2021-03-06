<?php
// $Id: node_resource.inc,v 1.1 2010/03/08 20:01:41 hugowetterberg Exp $


function _user_resource_definition() {
  return array(
    'user' => array(
      'retrieve' => array(
        'help' => 'Retrieves a user',
        'callback' => '_user_resource_retrieve',
        'file' => array('type' => 'inc', 'module' => 'services', 'name' => 'resources/user_resource'),
        'access callback' => '_user_resource_access',
        'access arguments' => array('view'),
        'access arguments append' => TRUE,
        'args' => array(
          array(
            'name' => 'uid',
            'type' => 'int',
            'description' => 'The uid of the user to retrieve.',
            'source' => array('path' => '0'),
            'optional' => FALSE,
          ),
        ),
      ),

      'create' => array(
        'help' => 'Creates a user',
        'callback' => '_user_resource_create',
        'file' => array('type' => 'inc', 'module' => 'services', 'name' => 'resources/user_resource'),
        'access callback' => '_user_resource_access',
        'access arguments' => array('create'),
        'access arguments append' => FALSE,
        'args' => array(
          array(
            'name' => 'account',
            'type' => 'struct',
            'description' => 'The user object',
            'source' => 'data',
            'optional' => FALSE,
          ),
        ),
      ),

      'update' => array(
        'help' => 'Updates a user',
        'callback' => '_user_resource_update',
        'file' => array('type' => 'inc', 'module' => 'services', 'name' => 'resources/user_resource'),
        'access callback' => '_user_resource_access',
        'access arguments' => array('update'),
        'access arguments append' => TRUE,
        'args' => array(
          array(
            'name' => 'uid',
            'type' => 'int',
            'description' => 'Unique identifier for this user',
            'source' => array('path' => 0),
            'optional' => FALSE,
          ),
          array(
            'name' => 'data',
            'type' => 'struct',
            'description' => 'The user object with updated information',
            'source' => 'data',
            'optional' => FALSE,
          ),
        ),
      ),
     
      'delete' => array(
        'help' => 'Deletes a user',
        'callback' => '_user_resource_delete',
        'file' => array('type' => 'inc', 'module' => 'services', 'name' => 'resources/user_resource'),
        'access callback' => '_user_resource_access',
        'access arguments' => array('delete'),
        'access arguments append' => FALSE,
        'args' => array(
          array(
            'name' => 'nid',
            'type' => 'int',
            'description' => 'The id of the note to delete',
            'source' => array('path' => '0'),
            'optional' => FALSE,
          ),
        ),
      ),
      'index' => array(
        'file' => array('type' => 'inc', 'module' => 'services', 'name' => 'resources/user_resource'),
        'callback' => '_user_resource_index',
        'args' => array(
          array(
            'name' => 'page',
            'optional' => TRUE,
            'type' => 'int',
            'description' => 'The zero-based index of the page to get, defaults to 0.',
            'default value' => 0,
            'source' => array('param' => 'page'),
          ),
          array(
            'name' => 'fields',
            'optional' => TRUE,
            'type' => 'string',
            'description' => 'The fields to get.',
            'default value' => '*',
            'source' => array('param' => 'fields'),
          ),
          array(
            'name' => 'parameters',
            'optional' => TRUE,
            'type' => 'struct',
            'description' => 'Parameters',
            'default value' => NULL,
            'source' => array('param' => 'parameters'),
          ),
        ),
        'access arguments' => array('access user profiles'),
        'access arguments append' => FALSE,
      ),
      'actions' => array(
        'login' => array(
          'help' => 'Login a user for a new session',
          'callback' => '_user_resource_login',
          'args' => array(
            array(
              'name' => 'username',
              'type' => 'string',
              'description' => 'A valid username',
              'source' => array('data' => 'name'),
              'optional' => FALSE,
            ),
            array(
              'name' => 'password',
              'type' => 'string',
              'description' => 'A valid password',
              'source' => array('data' => 'pass'),
              'optional' => FALSE,
            ),
          ),
          'access callback' => 'services_access_menu',
          'file' => array('type' => 'inc', 'module' => 'services', 'name' => 'resources/user_resource'),
        ),

        'logout' => array(
          'file' => array('type' => 'inc', 'module' => 'services', 'name' => 'resources/user_resource'),
          'help' => 'Logout a user session',
          'callback' => '_user_resource_logout',
          'access callback' => 'services_access_menu',
        ),
      ),
    ),
  );
}

/**
 * Get user details.
 *
 * @param $uid
 *   UID of the user to be loaded.
 *
 * @return
 *   A user object.
 *
 * @see user_load()
 */
function _user_resource_retrieve($uid) {
  $account = user_load($uid);
  if (empty($account)) {
    return services_error(t('There is no user with such ID.'), 404);
  }

  // Everything went right.
  return $account;
}

/**
 * Create a new user.
 *
 * This function uses drupal_execute() and as such exepects all input to match 
 * the submitting form in question.
 *
 * @param $account
 *   A object containing account information. The $account object should 
 *   contain, at minimum, the following properties:
 *     - name (user name)
 *     - mail (email address)
 *     - pass (plain text unencrypted password)
 *
 *   These properties can be passed but are optional
 *     - status (0 for blocked, otherwise will be active by default)
 *     - notify (1 to notify user of new account, will not notify by default)
 *
 *  Roles can be passed in a roles property which is an associative
 *  array formatted with '<role id>' => '<role id>', not including
 *  the authenticated user role, which is given by default. 
 *  
 * @return
 *   The user object of the newly created user.
 */
function _user_resource_create($account) {
  // Load the required includes for saving profile information
  // with drupal_execute().
  module_load_include('inc', 'user', 'user.pages');

  // Any logged in user is by default authenticated,
  // and leaving this role set in the user's roles array
  // causes big problems because of a FAPI hack that controls
  // this checkbox on the user create and edit form (and thus
  // causes problems with drupal_execute()). Therefore we just
  // force it to 0 here.
  if (isset($account->roles[2])) {
    $account->roles[2] = 0;
  }

  // register a new user
  $form_state['values'] = (array) $account;
  $form_state['values']['pass'] = array(
    'pass1' => $account->pass,
    'pass2' => $account->pass,
  );
  $form_state['values']['op'] = t('Create new account');
  
  $ret = drupal_execute('user_register', $form_state);

  // Error if needed.
  if ($errors = form_get_errors()) {
    return services_error(implode("\n", $errors), 401);
  }
  else {
    return $form_state['user'];
  }
}

/**
 * Update an existing user.
 *
 * This function uses drupal_execute() and as such exepects all input to match 
 * the submitting form in question.
 *
 * @param $uid
 *   Unique identifier for this user
 * @param $account
 *   Fields to modify for this user.
 *
 * @return
 *   The modified user object.
 */
function _user_resource_update($uid, $account) {
  $account->uid = $uid;

  // Load the required includes for saving profile information
  // with drupal_execute().
  module_load_include('inc', 'user', 'user.pages');

  // Any logged in user is by default authenticated,
  // and leaving this role set in the user's roles array
  // causes big problems because of a FAPI hack that controls
  // this checkbox on the user create and edit form (and thus
  // causes problems with drupal_execute()). Therefore we just
  // force it to 0 here.
  if (isset($account->roles[2])) {
    $account->roles[2] = 0;
  }

  // If a profile category was passed in, use it. Otherwise default
  // to 'account' (for saving core user data.)
  $category = 'account';
  if (isset($account->category)) {
    $category = $account->category;
    unset($account->category);
  }
  
  // Drop any passed in values into the $account var. Anything
  // unused by the form just gets ignored.
  foreach ($account as $key => $value) {
    $form_state['values'][$key] = $value;
  }

  $form_state['values']['op'] = 'Save';
  $form_state['values']['_category'] = $category;
  $form_state['values']['_account'] = $account;

  $ret = drupal_execute('user_profile_form', $form_state, $account, $category);

  // Error if needed.
  if ($errors = form_get_errors()) {
    return services_error(implode("\n", $errors), 401);
  }
  else {
    return $account;
  }
}

/**
 * Delete a user.
 *
 * @param $uid
 *   UID of the user to be deleted.
 *
 * @see user_delete()
 */
function _user_resource_delete($uid) {
  $account = user_load($uid);
  if (empty($account)) {
    return services_error(t('There is no user with such ID.'), 404);
  }
  user_delete($account, $uid);

  // Everything went right.
  return TRUE;
}

/**
 * Login a user using the specified credentials.
 *
 * Note this will transfer a plaintext password.
 *
 * @param $username
 *   Username to be logged in.
 * @param $password
 *   Password, must be plain text and not hashed.
 *
 * @return
 *   A valid session object.
 */
function _user_resource_login($username, $password) {
  global $user;

  if ($user->uid) {
    // user is already logged in
    return services_error(t('Already logged in as !user.', array('!user' => $user->name)), 406);
  }

  $user = user_authenticate(array('name' => $username, 'pass' => $password));

  if ($user->uid) {
    // Regenerate the session ID to prevent against session fixation attacks.
    sess_regenerate();
    module_invoke_all('user', 'login', NULL, $user);

    $return = new stdClass();
    $return->sessid = session_id();
    $return->session_name = session_name();
    $return->user = $user;

    return $return;
  }
  session_destroy();
  return services_error(t('Wrong username or password.'), 401);
}

/**
 * Logout the current user.
 */
function _user_resource_logout() {
  global $user;

  if (!$user->uid) {
    // User is not logged in
    return services_error(t('User is not logged in.'), 406);
  }

  watchdog('user', 'Session closed for %name.', array('%name' => theme('placeholder', $user->name)));

  // Destroy the current session:
  session_destroy();
  module_invoke_all('user', 'logout', NULL, $user);

  // Load the anonymous user
  $user = drupal_anonymous_user();

  return TRUE;
}

/**
 * Return an array of optionally paged nids baed on a set of criteria.
 *
 * An example request might look like
 *
 * http://domain/endpoint/user?fields=uid,name,mail&parameters[uid]=1
 *
 * This would return an array of objects with only uid, name and mail defined,
 * where uid = 1.
 *
 * @param $page
 *   Page number of results to return (in pages of 20).
 * @param $fields
 *   The fields you want returned. 
 * @param $parameters
 *   An array of fields and values used to build a sql WHERE clause indicating
 *   what items should be deleted.
 * @return
 *   An array of user objects.
 *
 * @see _node_resource_index() for more notes
 **/
function _user_resource_index($page = 0, $fields = '*', $parameters = array()) {
  $parameters = (array) $parameters;
  $schema = drupal_get_schema('node');
  $where = array();
  $fields = db_escape_string($fields);

  $parameters = (array)$parameters;
  if (!user_access('administer users')) {
    $parameters['active'] = 1;
  }
 
  // Build an array of fields with the appropriate placeholders for use in
  // db_query().
  foreach ($parameters as $field => $value) {
    $where[] = $field . ' = ' . db_type_placeholder($schema['fields'][$field]['type']);
  }

  // Now implode that array into an actual WHERE clause.
  $where = !empty($where) ? ' WHERE '. db_escape_string(implode(' AND ', $where)) : '';
  $sql = "SELECT $fields FROM {users} $where";
  $result = db_query_range($sql, $parameters, $page * 20, 20);

  // Put together array of matching accounts to return.
  $accounts = array();
  while ($account = db_fetch_object($result)) {
    if ($uri = services_resource_uri(array('user', $account->uid))) {
      $account->uri = $uri;
    }
    $accounts[$account->uid] = $account;
  }
  return $accounts;
}

/**
 * Access check callback for user resource.
 */
function _user_resource_access($op = 'view', $args = array()) {
  global $user;
  switch($op) {
    case 'view':
      $account = user_load($args[0]);
      return user_view_access($account);
    case 'update':
      $args[0] = $account;
      return ($user->uid == $account->uid || user_access('administer users'));
    case 'create':
    case 'delete':
      return user_access('administer users');
  }
}
