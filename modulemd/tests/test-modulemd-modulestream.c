#include <glib.h>
#include <glib/gstdio.h>
#include <locale.h>
#include <signal.h>

#include "modulemd-module-index.h"
#include "modulemd-module-stream.h"
#include "private/glib-extensions.h"
#include "private/modulemd-module-stream-private.h"
#include "private/modulemd-module-stream-v1-private.h"
#include "private/modulemd-module-stream-v2-private.h"
#include "private/modulemd-subdocument-info-private.h"
#include "private/modulemd-util.h"
#include "private/modulemd-yaml.h"
#include "private/test-utils.h"

#define MMD_TEST_DOC_TEXT "http://example.com"
#define MMD_TEST_DOC_TEXT2 "http://redhat.com"
#define MMD_TEST_DOC_PROP "documentation"
#define MMD_TEST_COM_PROP "community"
#define MMD_TEST_DOC_UNICODE_TEXT                                             \
  "À϶￥🌭∮⇒⇔¬β∀₂⌀ıəˈ⍳⍴V)"                           \
  "═€ίζησθლბშიнстемองจึองታሽ።ደለᚢᛞᚦᚹ⠳⠞⠊⠎▉▒▒▓😃"
#define MMD_TEST_TRACKER_PROP "tracker"

typedef struct _ModuleStreamFixture
{
} ModuleStreamFixture;


static void
module_stream_test_construct (ModuleStreamFixture *fixture,
                              gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  guint64 version;

  for (version = MD_MODULESTREAM_VERSION_ONE;
       version <= MD_MODULESTREAM_VERSION_LATEST;
       version++)
    {
      /* Test that the new() function works */
      stream = modulemd_module_stream_new (version, "foo", "latest");
      g_assert_nonnull (stream);
      g_assert_true (MODULEMD_IS_MODULE_STREAM (stream));

      g_assert_cmpint (
        modulemd_module_stream_get_mdversion (stream), ==, version);
      g_assert_cmpstr (
        modulemd_module_stream_get_module_name (stream), ==, "foo");
      g_assert_cmpstr (
        modulemd_module_stream_get_stream_name (stream), ==, "latest");

      g_clear_object (&stream);

      /* Test that the new() function works without a stream name */
      stream = modulemd_module_stream_new (version, "foo", NULL);
      g_assert_nonnull (stream);
      g_assert_true (MODULEMD_IS_MODULE_STREAM (stream));

      g_assert_cmpint (
        modulemd_module_stream_get_mdversion (stream), ==, version);
      g_assert_cmpstr (
        modulemd_module_stream_get_module_name (stream), ==, "foo");
      g_assert_null (modulemd_module_stream_get_stream_name (stream));

      g_clear_object (&stream);

      /* Test with no module name */
      stream = modulemd_module_stream_new (version, NULL, NULL);
      g_assert_nonnull (stream);
      g_assert_true (MODULEMD_IS_MODULE_STREAM (stream));

      g_assert_cmpint (
        modulemd_module_stream_get_mdversion (stream), ==, version);
      g_assert_null (modulemd_module_stream_get_module_name (stream));
      g_assert_null (modulemd_module_stream_get_stream_name (stream));

      g_clear_object (&stream);
    }

  /* Test with a zero mdversion */
  stream = modulemd_module_stream_new (0, "foo", "latest");
  g_assert_null (stream);
  g_clear_object (&stream);


  /* Test with an unknown mdversion */
  stream = modulemd_module_stream_new (
    MD_MODULESTREAM_VERSION_LATEST + 1, "foo", "latest");
  g_assert_null (stream);
  g_clear_object (&stream);
}


static void
module_stream_test_arch (ModuleStreamFixture *fixture, gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  guint64 version;
  g_autofree gchar *arch = NULL;

  for (version = MD_MODULESTREAM_VERSION_ONE;
       version <= MD_MODULESTREAM_VERSION_LATEST;
       version++)
    {
      /* Test the parent class set_arch() and get_arch() */
      stream = modulemd_module_stream_new (version, "foo", "latest");
      g_assert_nonnull (stream);

      g_assert_null (modulemd_module_stream_get_arch (stream));

      // clang-format off
      g_object_get (stream,
                    "arch", &arch,
                    NULL);
      // clang-format on
      g_assert_null (arch);

      modulemd_module_stream_set_arch (stream, "x86_64");
      g_assert_cmpstr (modulemd_module_stream_get_arch (stream), ==, "x86_64");

      // clang-format off
      g_object_set (stream,
                    "arch", "aarch64",
                    NULL);
      g_object_get (stream,
                    "arch", &arch,
                    NULL);
      // clang-format on
      g_assert_cmpstr (arch, ==, "aarch64");
      g_clear_pointer (&arch, g_free);

      g_clear_object (&stream);
    }
}


static void
module_stream_v1_test_licenses (ModuleStreamFixture *fixture,
                                gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV1) stream = NULL;
  g_auto (GStrv) licenses = NULL;

  stream = modulemd_module_stream_v1_new (NULL, NULL);

  modulemd_module_stream_v1_add_content_license (stream, "GPLv2+");
  licenses = modulemd_module_stream_v1_get_content_licenses_as_strv (stream);
  g_assert_true (g_strv_contains ((const gchar *const *)licenses, "GPLv2+"));
  g_assert_cmpint (g_strv_length (licenses), ==, 1);

  g_clear_pointer (&licenses, g_strfreev);

  modulemd_module_stream_v1_add_module_license (stream, "MIT");
  licenses = modulemd_module_stream_v1_get_module_licenses_as_strv (stream);
  g_assert_true (g_strv_contains ((const gchar *const *)licenses, "MIT"));
  g_assert_cmpint (g_strv_length (licenses), ==, 1);

  g_clear_pointer (&licenses, g_strfreev);

  modulemd_module_stream_v1_remove_content_license (stream, "GPLv2+");
  licenses = modulemd_module_stream_v1_get_content_licenses_as_strv (stream);
  g_assert_cmpint (g_strv_length (licenses), ==, 0);

  g_clear_pointer (&licenses, g_strfreev);

  modulemd_module_stream_v1_remove_module_license (stream, "MIT");
  licenses = modulemd_module_stream_v1_get_module_licenses_as_strv (stream);
  g_assert_cmpint (g_strv_length (licenses), ==, 0);

  g_clear_pointer (&licenses, g_strfreev);
  g_clear_object (&stream);
}

static void
module_stream_v2_test_licenses (ModuleStreamFixture *fixture,
                                gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  g_auto (GStrv) licenses = NULL;

  stream = modulemd_module_stream_v2_new (NULL, NULL);

  modulemd_module_stream_v2_add_content_license (stream, "GPLv2+");
  licenses = modulemd_module_stream_v2_get_content_licenses_as_strv (stream);
  g_assert_true (g_strv_contains ((const gchar *const *)licenses, "GPLv2+"));
  g_assert_cmpint (g_strv_length (licenses), ==, 1);

  g_clear_pointer (&licenses, g_strfreev);

  modulemd_module_stream_v2_add_module_license (stream, "MIT");
  licenses = modulemd_module_stream_v2_get_module_licenses_as_strv (stream);
  g_assert_true (g_strv_contains ((const gchar *const *)licenses, "MIT"));
  g_assert_cmpint (g_strv_length (licenses), ==, 1);

  g_clear_pointer (&licenses, g_strfreev);

  modulemd_module_stream_v2_remove_content_license (stream, "GPLv2+");
  licenses = modulemd_module_stream_v2_get_content_licenses_as_strv (stream);
  g_assert_cmpint (g_strv_length (licenses), ==, 0);

  g_clear_pointer (&licenses, g_strfreev);

  modulemd_module_stream_v2_remove_module_license (stream, "MIT");
  licenses = modulemd_module_stream_v2_get_module_licenses_as_strv (stream);
  g_assert_cmpint (g_strv_length (licenses), ==, 0);

  g_clear_pointer (&licenses, g_strfreev);
  g_clear_object (&stream);
}


static void
module_stream_v1_test_profiles (ModuleStreamFixture *fixture,
                                gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV1) stream = NULL;
  g_autoptr (ModulemdProfile) profile = NULL;
  g_auto (GStrv) profiles = NULL;
  g_auto (GStrv) rpms = NULL;

  stream = modulemd_module_stream_v1_new ("sssd", NULL);

  profile = modulemd_profile_new ("client");
  modulemd_profile_add_rpm (profile, "sssd-client");

  modulemd_module_stream_v1_add_profile (stream, profile);
  profiles = modulemd_module_stream_v1_get_profile_names_as_strv (stream);
  g_assert_cmpint (g_strv_length (profiles), ==, 1);
  g_assert_true (g_strv_contains ((const gchar *const *)profiles, "client"));

  g_clear_pointer (&profiles, g_strfreev);

  rpms = modulemd_profile_get_rpms_as_strv (
    modulemd_module_stream_v1_get_profile (stream, "client"));
  g_assert_true (g_strv_contains ((const gchar *const *)rpms, "sssd-client"));

  modulemd_module_stream_v1_clear_profiles (stream);
  profiles = modulemd_module_stream_v1_get_profile_names_as_strv (stream);
  g_assert_cmpint (g_strv_length (profiles), ==, 0);

  g_clear_object (&stream);
  g_clear_object (&profile);
  g_clear_pointer (&profiles, g_strfreev);
  g_clear_pointer (&rpms, g_strfreev);
}


static void
module_stream_v2_test_profiles (ModuleStreamFixture *fixture,
                                gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  g_autoptr (ModulemdProfile) profile = NULL;
  g_auto (GStrv) profiles = NULL;
  g_auto (GStrv) rpms = NULL;

  stream = modulemd_module_stream_v2_new ("sssd", NULL);

  profile = modulemd_profile_new ("client");
  modulemd_profile_add_rpm (profile, "sssd-client");

  modulemd_module_stream_v2_add_profile (stream, profile);
  profiles = modulemd_module_stream_v2_get_profile_names_as_strv (stream);
  g_assert_cmpint (g_strv_length (profiles), ==, 1);
  g_assert_true (g_strv_contains ((const gchar *const *)profiles, "client"));

  g_clear_pointer (&profiles, g_strfreev);

  rpms = modulemd_profile_get_rpms_as_strv (
    modulemd_module_stream_v2_get_profile (stream, "client"));
  g_assert_true (g_strv_contains ((const gchar *const *)rpms, "sssd-client"));

  modulemd_module_stream_v2_clear_profiles (stream);
  profiles = modulemd_module_stream_v2_get_profile_names_as_strv (stream);
  g_assert_cmpint (g_strv_length (profiles), ==, 0);

  g_clear_object (&stream);
  g_clear_object (&profile);
  g_clear_pointer (&profiles, g_strfreev);
  g_clear_pointer (&rpms, g_strfreev);
}


static void
module_stream_v1_test_rpm_api (ModuleStreamFixture *fixture,
                               gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV1) stream = NULL;
  g_auto (GStrv) rpm_apis = NULL;

  stream = modulemd_module_stream_v1_new ("sssd", NULL);

  modulemd_module_stream_v1_add_rpm_api (stream, "sssd-common");
  rpm_apis = modulemd_module_stream_v1_get_rpm_api_as_strv (stream);

  g_assert_true (
    g_strv_contains ((const gchar *const *)rpm_apis, "sssd-common"));
  g_assert_cmpint (g_strv_length (rpm_apis), ==, 1);

  g_clear_pointer (&rpm_apis, g_strfreev);

  modulemd_module_stream_v1_remove_rpm_api (stream, "sssd-common");
  rpm_apis = modulemd_module_stream_v1_get_rpm_api_as_strv (stream);

  g_assert_cmpint (g_strv_length (rpm_apis), ==, 0);

  g_clear_pointer (&rpm_apis, g_strfreev);
  g_clear_object (&stream);
}


static void
module_stream_v2_test_rpm_api (ModuleStreamFixture *fixture,
                               gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  g_auto (GStrv) rpm_apis = NULL;

  stream = modulemd_module_stream_v2_new ("sssd", NULL);

  modulemd_module_stream_v2_add_rpm_api (stream, "sssd-common");
  rpm_apis = modulemd_module_stream_v2_get_rpm_api_as_strv (stream);

  g_assert_true (
    g_strv_contains ((const gchar *const *)rpm_apis, "sssd-common"));
  g_assert_cmpint (g_strv_length (rpm_apis), ==, 1);

  g_clear_pointer (&rpm_apis, g_strfreev);

  modulemd_module_stream_v2_remove_rpm_api (stream, "sssd-common");
  rpm_apis = modulemd_module_stream_v2_get_rpm_api_as_strv (stream);

  g_assert_cmpint (g_strv_length (rpm_apis), ==, 0);

  g_clear_pointer (&rpm_apis, g_strfreev);
  g_clear_object (&stream);
}


static void
module_stream_v1_test_rpm_filters (ModuleStreamFixture *fixture,
                                   gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV1) stream = NULL;
  g_auto (GStrv) filters = NULL;

  stream = modulemd_module_stream_v1_new ("sssd", NULL);

  // Test add_rpm_filter
  modulemd_module_stream_v1_add_rpm_filter (stream, "foo");
  modulemd_module_stream_v1_add_rpm_filter (stream, "bar");
  filters = modulemd_module_stream_v1_get_rpm_filters_as_strv (stream);

  g_assert_true (g_strv_contains ((const gchar *const *)filters, "foo"));
  g_assert_true (g_strv_contains ((const gchar *const *)filters, "bar"));
  g_assert_cmpint (g_strv_length (filters), ==, 2);
  g_clear_pointer (&filters, g_strfreev);

  // Test remove_rpm_filter
  modulemd_module_stream_v1_remove_rpm_filter (stream, "foo");
  filters = modulemd_module_stream_v1_get_rpm_filters_as_strv (stream);

  g_assert_true (g_strv_contains ((const gchar *const *)filters, "bar"));
  g_assert_cmpint (g_strv_length (filters), ==, 1);
  g_clear_pointer (&filters, g_strfreev);

  // Test clear_rpm_filters
  modulemd_module_stream_v1_clear_rpm_filters (stream);
  filters = modulemd_module_stream_v1_get_rpm_filters_as_strv (stream);
  g_assert_cmpint (g_strv_length (filters), ==, 0);

  g_clear_pointer (&filters, g_strfreev);
  g_clear_object (&stream);
}

static void
module_stream_v2_test_rpm_filters (ModuleStreamFixture *fixture,
                                   gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  g_auto (GStrv) filters = NULL;

  stream = modulemd_module_stream_v2_new ("sssd", NULL);

  // Test add_rpm_filter
  modulemd_module_stream_v2_add_rpm_filter (stream, "foo");
  modulemd_module_stream_v2_add_rpm_filter (stream, "bar");
  filters = modulemd_module_stream_v2_get_rpm_filters_as_strv (stream);

  g_assert_true (g_strv_contains ((const gchar *const *)filters, "foo"));
  g_assert_true (g_strv_contains ((const gchar *const *)filters, "bar"));
  g_assert_cmpint (g_strv_length (filters), ==, 2);
  g_clear_pointer (&filters, g_strfreev);

  // Test remove_rpm_filter
  modulemd_module_stream_v2_remove_rpm_filter (stream, "foo");
  filters = modulemd_module_stream_v2_get_rpm_filters_as_strv (stream);

  g_assert_true (g_strv_contains ((const gchar *const *)filters, "bar"));
  g_assert_cmpint (g_strv_length (filters), ==, 1);
  g_clear_pointer (&filters, g_strfreev);

  // Test clear_rpm_filters
  modulemd_module_stream_v2_clear_rpm_filters (stream);
  filters = modulemd_module_stream_v2_get_rpm_filters_as_strv (stream);
  g_assert_cmpint (g_strv_length (filters), ==, 0);

  g_clear_pointer (&filters, g_strfreev);
  g_clear_object (&stream);
}

static void
module_stream_test_upgrade (ModuleStreamFixture *fixture,
                            gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV1) streamV1 = NULL;
  g_autoptr (ModulemdModuleStream) updated_stream = NULL;
  g_autoptr (ModulemdModuleIndex) index = NULL;
  g_autoptr (GError) error = NULL;
  g_autofree gchar *yaml_str = NULL;

  streamV1 = modulemd_module_stream_v1_new ("SuperModule", "latest");

  modulemd_module_stream_v1_set_summary (streamV1, "Summary");
  modulemd_module_stream_v1_set_description (streamV1, "Description");
  modulemd_module_stream_v1_add_module_license (streamV1, "BSD");

  modulemd_module_stream_v1_add_buildtime_requirement (
    streamV1, "ModuleA", "streamZ");
  modulemd_module_stream_v1_add_buildtime_requirement (
    streamV1, "ModuleB", "streamY");
  modulemd_module_stream_v1_add_runtime_requirement (
    streamV1, "ModuleA", "streamZ");
  modulemd_module_stream_v1_add_runtime_requirement (
    streamV1, "ModuleB", "streamY");

  updated_stream = modulemd_module_stream_upgrade (
    MODULEMD_MODULE_STREAM (streamV1), MD_MODULESTREAM_VERSION_LATEST, &error);

  g_assert_no_error (error);
  g_assert_nonnull (updated_stream);

  index = modulemd_module_index_new ();
  modulemd_module_index_add_module_stream (
    index, MODULEMD_MODULE_STREAM (updated_stream), &error);

  g_assert_no_error (error);

  yaml_str = modulemd_module_index_dump_to_string (index, &error);

  g_assert_no_error (error);
  g_assert_cmpstr (yaml_str,
                   ==,
                   "---\n"
                   "document: modulemd\n"
                   "version: 2\n"
                   "data:\n"
                   "  name: SuperModule\n"
                   "  stream: latest\n"
                   "  summary: Summary\n"
                   "  description: >-\n"
                   "    Description\n"
                   "  license:\n"
                   "    module:\n"
                   "    - BSD\n"
                   "  dependencies:\n"
                   "  - buildrequires:\n"
                   "      ModuleA: [streamZ]\n"
                   "      ModuleB: [streamY]\n"
                   "    requires:\n"
                   "      ModuleA: [streamZ]\n"
                   "      ModuleB: [streamY]\n"
                   "...\n");

  g_clear_object (&streamV1);
  g_clear_object (&updated_stream);
  g_clear_object (&index);
  g_clear_object (&error);
  g_clear_pointer (&yaml_str, g_free);
}

static void
module_stream_v1_test_rpm_artifacts (ModuleStreamFixture *fixture,
                                     gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV1) stream = NULL;
  g_auto (GStrv) artifacts = NULL;

  stream = modulemd_module_stream_v1_new (NULL, NULL);

  modulemd_module_stream_v1_add_rpm_artifact (
    stream, "bar-0:1.23-1.module_deadbeef.x86_64");
  artifacts = modulemd_module_stream_v1_get_rpm_artifacts_as_strv (stream);
  g_assert_true (g_strv_contains ((const gchar *const *)artifacts,
                                  "bar-0:1.23-1.module_deadbeef.x86_64"));
  g_assert_cmpint (g_strv_length (artifacts), ==, 1);

  g_clear_pointer (&artifacts, g_strfreev);

  modulemd_module_stream_v1_remove_rpm_artifact (
    stream, "bar-0:1.23-1.module_deadbeef.x86_64");
  artifacts = modulemd_module_stream_v1_get_rpm_artifacts_as_strv (stream);
  g_assert_cmpint (g_strv_length (artifacts), ==, 0);

  g_clear_pointer (&artifacts, g_strfreev);
  g_clear_object (&stream);
}


static void
module_stream_v2_test_rpm_artifacts (ModuleStreamFixture *fixture,
                                     gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  g_auto (GStrv) artifacts = NULL;

  stream = modulemd_module_stream_v2_new (NULL, NULL);

  modulemd_module_stream_v2_add_rpm_artifact (
    stream, "bar-0:1.23-1.module_deadbeef.x86_64");
  artifacts = modulemd_module_stream_v2_get_rpm_artifacts_as_strv (stream);
  g_assert_true (g_strv_contains ((const gchar *const *)artifacts,
                                  "bar-0:1.23-1.module_deadbeef.x86_64"));
  g_assert_cmpint (g_strv_length (artifacts), ==, 1);

  g_clear_pointer (&artifacts, g_strfreev);

  modulemd_module_stream_v2_remove_rpm_artifact (
    stream, "bar-0:1.23-1.module_deadbeef.x86_64");
  artifacts = modulemd_module_stream_v2_get_rpm_artifacts_as_strv (stream);
  g_assert_cmpint (g_strv_length (artifacts), ==, 0);

  g_clear_pointer (&artifacts, g_strfreev);
  g_clear_object (&stream);
}

static void
module_stream_v1_test_documentation (ModuleStreamFixture *fixture,
                                     gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV1) stream = NULL;
  const gchar *documentation = NULL;
  g_autofree gchar *documentation_prop = NULL;

  stream = modulemd_module_stream_v1_new (NULL, NULL);

  // Check the defaults
  documentation = modulemd_module_stream_v1_get_documentation (stream);
  g_object_get (stream, MMD_TEST_DOC_PROP, &documentation_prop, NULL);
  g_assert_null (documentation);
  g_assert_null (documentation_prop);

  g_clear_pointer (&documentation_prop, g_free);

  // Test property setting
  g_object_set (stream, MMD_TEST_DOC_PROP, MMD_TEST_DOC_TEXT, NULL);

  documentation = modulemd_module_stream_v1_get_documentation (stream);
  g_object_get (stream, MMD_TEST_DOC_PROP, &documentation_prop, NULL);
  g_assert_cmpstr (documentation_prop, ==, MMD_TEST_DOC_TEXT);
  g_assert_cmpstr (documentation, ==, MMD_TEST_DOC_TEXT);

  g_clear_pointer (&documentation_prop, g_free);

  // Test set_documentation()
  modulemd_module_stream_v1_set_documentation (stream, MMD_TEST_DOC_TEXT2);

  documentation = modulemd_module_stream_v1_get_documentation (stream);
  g_object_get (stream, MMD_TEST_DOC_PROP, &documentation_prop, NULL);
  g_assert_cmpstr (documentation_prop, ==, MMD_TEST_DOC_TEXT2);
  g_assert_cmpstr (documentation, ==, MMD_TEST_DOC_TEXT2);

  g_clear_pointer (&documentation_prop, g_free);

  // Test setting to NULL
  g_object_set (stream, MMD_TEST_DOC_PROP, NULL, NULL);

  documentation = modulemd_module_stream_v1_get_documentation (stream);
  g_object_get (stream, MMD_TEST_DOC_PROP, &documentation_prop, NULL);
  g_assert_null (documentation);
  g_assert_null (documentation_prop);

  g_clear_pointer (&documentation_prop, g_free);

  // Test unicode characters
  modulemd_module_stream_v1_set_documentation (stream,
                                               MMD_TEST_DOC_UNICODE_TEXT);

  documentation = modulemd_module_stream_v1_get_documentation (stream);
  g_object_get (stream, MMD_TEST_DOC_PROP, &documentation_prop, NULL);
  g_assert_cmpstr (documentation_prop, ==, MMD_TEST_DOC_UNICODE_TEXT);
  g_assert_cmpstr (documentation, ==, MMD_TEST_DOC_UNICODE_TEXT);

  g_clear_pointer (&documentation_prop, g_free);

  g_clear_object (&stream);
}


static void
module_stream_v2_test_documentation (ModuleStreamFixture *fixture,
                                     gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  const gchar *documentation = NULL;
  g_autofree gchar *documentation_prop = NULL;

  stream = modulemd_module_stream_v2_new (NULL, NULL);

  // Check the defaults
  documentation = modulemd_module_stream_v2_get_documentation (stream);
  g_object_get (stream, MMD_TEST_DOC_PROP, &documentation_prop, NULL);
  g_assert_null (documentation);
  g_assert_null (documentation_prop);

  g_clear_pointer (&documentation_prop, g_free);

  // Test property setting
  g_object_set (stream, MMD_TEST_DOC_PROP, MMD_TEST_DOC_TEXT, NULL);

  documentation = modulemd_module_stream_v2_get_documentation (stream);
  g_object_get (stream, MMD_TEST_DOC_PROP, &documentation_prop, NULL);
  g_assert_cmpstr (documentation_prop, ==, MMD_TEST_DOC_TEXT);
  g_assert_cmpstr (documentation, ==, MMD_TEST_DOC_TEXT);

  g_clear_pointer (&documentation_prop, g_free);

  // Test set_documentation()
  modulemd_module_stream_v2_set_documentation (stream, MMD_TEST_DOC_TEXT2);

  documentation = modulemd_module_stream_v2_get_documentation (stream);
  g_object_get (stream, MMD_TEST_DOC_PROP, &documentation_prop, NULL);
  g_assert_cmpstr (documentation_prop, ==, MMD_TEST_DOC_TEXT2);
  g_assert_cmpstr (documentation, ==, MMD_TEST_DOC_TEXT2);

  g_clear_pointer (&documentation_prop, g_free);

  // Test setting to NULL
  g_object_set (stream, MMD_TEST_DOC_PROP, NULL, NULL);

  documentation = modulemd_module_stream_v2_get_documentation (stream);
  g_object_get (stream, MMD_TEST_DOC_PROP, &documentation_prop, NULL);
  g_assert_null (documentation);
  g_assert_null (documentation_prop);

  g_clear_pointer (&documentation_prop, g_free);

  // Test unicode characters
  modulemd_module_stream_v2_set_documentation (stream,
                                               MMD_TEST_DOC_UNICODE_TEXT);

  documentation = modulemd_module_stream_v2_get_documentation (stream);
  g_object_get (stream, MMD_TEST_DOC_PROP, &documentation_prop, NULL);
  g_assert_cmpstr (documentation_prop, ==, MMD_TEST_DOC_UNICODE_TEXT);
  g_assert_cmpstr (documentation, ==, MMD_TEST_DOC_UNICODE_TEXT);

  g_clear_pointer (&documentation_prop, g_free);

  g_clear_object (&stream);
}

static void
module_stream_v1_test_tracker (ModuleStreamFixture *fixture,
                               gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV1) stream = NULL;
  g_autofree gchar *tracker_prop = NULL;
  const gchar *tracker = NULL;

  stream = modulemd_module_stream_v1_new (NULL, NULL);

  // Check the defaults
  g_object_get (stream, MMD_TEST_TRACKER_PROP, &tracker_prop, NULL);
  tracker = modulemd_module_stream_v1_get_tracker (stream);

  g_assert_null (tracker);
  g_assert_null (tracker_prop);

  g_clear_pointer (&tracker_prop, g_free);

  // Test property setting
  g_object_set (stream, MMD_TEST_TRACKER_PROP, MMD_TEST_DOC_TEXT, NULL);

  g_object_get (stream, MMD_TEST_TRACKER_PROP, &tracker_prop, NULL);
  tracker = modulemd_module_stream_v1_get_tracker (stream);

  g_assert_cmpstr (tracker, ==, MMD_TEST_DOC_TEXT);
  g_assert_cmpstr (tracker_prop, ==, MMD_TEST_DOC_TEXT);

  g_clear_pointer (&tracker_prop, g_free);

  // Test set_tracker
  modulemd_module_stream_v1_set_tracker (stream, MMD_TEST_DOC_TEXT2);

  g_object_get (stream, MMD_TEST_TRACKER_PROP, &tracker_prop, NULL);
  tracker = modulemd_module_stream_v1_get_tracker (stream);

  g_assert_cmpstr (tracker, ==, MMD_TEST_DOC_TEXT2);
  g_assert_cmpstr (tracker_prop, ==, MMD_TEST_DOC_TEXT2);

  g_clear_pointer (&tracker_prop, g_free);

  // Test setting it to NULL
  g_object_set (stream, MMD_TEST_TRACKER_PROP, NULL, NULL);

  g_object_get (stream, MMD_TEST_TRACKER_PROP, &tracker_prop, NULL);
  tracker = modulemd_module_stream_v1_get_tracker (stream);

  g_assert_null (tracker);
  g_assert_null (tracker_prop);

  g_clear_pointer (&tracker_prop, g_free);

  // Test Unicode values
  modulemd_module_stream_v1_set_tracker (stream, MMD_TEST_DOC_UNICODE_TEXT);

  g_object_get (stream, MMD_TEST_TRACKER_PROP, &tracker_prop, NULL);
  tracker = modulemd_module_stream_v1_get_tracker (stream);

  g_assert_cmpstr (tracker, ==, MMD_TEST_DOC_UNICODE_TEXT);
  g_assert_cmpstr (tracker_prop, ==, MMD_TEST_DOC_UNICODE_TEXT);

  g_clear_pointer (&tracker_prop, g_free);
  g_clear_object (&stream);
}

static void
module_stream_v2_test_tracker (ModuleStreamFixture *fixture,
                               gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  g_autofree gchar *tracker_prop = NULL;
  const gchar *tracker = NULL;

  stream = modulemd_module_stream_v2_new (NULL, NULL);

  // Check the defaults
  g_object_get (stream, MMD_TEST_TRACKER_PROP, &tracker_prop, NULL);
  tracker = modulemd_module_stream_v2_get_tracker (stream);

  g_assert_null (tracker);
  g_assert_null (tracker_prop);

  g_clear_pointer (&tracker_prop, g_free);

  // Test property setting
  g_object_set (stream, MMD_TEST_TRACKER_PROP, MMD_TEST_DOC_TEXT, NULL);

  g_object_get (stream, MMD_TEST_TRACKER_PROP, &tracker_prop, NULL);
  tracker = modulemd_module_stream_v2_get_tracker (stream);

  g_assert_cmpstr (tracker, ==, MMD_TEST_DOC_TEXT);
  g_assert_cmpstr (tracker_prop, ==, MMD_TEST_DOC_TEXT);

  g_clear_pointer (&tracker_prop, g_free);

  // Test set_tracker
  modulemd_module_stream_v2_set_tracker (stream, MMD_TEST_DOC_TEXT2);

  g_object_get (stream, MMD_TEST_TRACKER_PROP, &tracker_prop, NULL);
  tracker = modulemd_module_stream_v2_get_tracker (stream);

  g_assert_cmpstr (tracker, ==, MMD_TEST_DOC_TEXT2);
  g_assert_cmpstr (tracker_prop, ==, MMD_TEST_DOC_TEXT2);

  g_clear_pointer (&tracker_prop, g_free);

  // Test setting it to NULL
  g_object_set (stream, MMD_TEST_TRACKER_PROP, NULL, NULL);

  g_object_get (stream, MMD_TEST_TRACKER_PROP, &tracker_prop, NULL);
  tracker = modulemd_module_stream_v2_get_tracker (stream);

  g_assert_null (tracker);
  g_assert_null (tracker_prop);

  g_clear_pointer (&tracker_prop, g_free);

  // Test Unicode values
  modulemd_module_stream_v2_set_tracker (stream, MMD_TEST_DOC_UNICODE_TEXT);

  g_object_get (stream, MMD_TEST_TRACKER_PROP, &tracker_prop, NULL);
  tracker = modulemd_module_stream_v2_get_tracker (stream);

  g_assert_cmpstr (tracker, ==, MMD_TEST_DOC_UNICODE_TEXT);
  g_assert_cmpstr (tracker_prop, ==, MMD_TEST_DOC_UNICODE_TEXT);

  g_clear_pointer (&tracker_prop, g_free);
  g_clear_object (&stream);
}

static void
module_stream_v1_test_components (ModuleStreamFixture *fixture,
                                  gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV1) stream = NULL;
  g_autoptr (ModulemdComponentRpm) rpm_component = NULL;
  g_autoptr (ModulemdComponentModule) module_component = NULL;
  ModulemdComponent *retrieved_component = NULL;
  g_auto (GStrv) component_names = NULL;

  stream = modulemd_module_stream_v1_new (NULL, NULL);

  // Add a RPM component to a stream
  rpm_component = modulemd_component_rpm_new ("rpmcomponent");
  modulemd_module_stream_v1_add_component (stream,
                                           (ModulemdComponent *)rpm_component);
  component_names =
    modulemd_module_stream_v1_get_rpm_component_names_as_strv (stream);
  g_assert_true (
    g_strv_contains ((const gchar *const *)component_names, "rpmcomponent"));
  g_assert_cmpint (g_strv_length (component_names), ==, 1);

  retrieved_component =
    (ModulemdComponent *)modulemd_module_stream_v1_get_rpm_component (
      stream, "rpmcomponent");
  g_assert_nonnull (retrieved_component);
  g_assert_true (modulemd_component_equals (
    retrieved_component, (ModulemdComponent *)rpm_component));

  g_clear_pointer (&component_names, g_strfreev);

  // Add a Module component to a stream
  module_component = modulemd_component_module_new ("modulecomponent");
  modulemd_module_stream_v1_add_component (
    stream, (ModulemdComponent *)module_component);
  component_names =
    modulemd_module_stream_v1_get_module_component_names_as_strv (stream);
  g_assert_true (g_strv_contains ((const gchar *const *)component_names,
                                  "modulecomponent"));
  g_assert_cmpint (g_strv_length (component_names), ==, 1);

  retrieved_component =
    (ModulemdComponent *)modulemd_module_stream_v1_get_module_component (
      stream, "modulecomponent");
  g_assert_nonnull (retrieved_component);
  g_assert_true (modulemd_component_equals (
    retrieved_component, (ModulemdComponent *)module_component));

  g_clear_pointer (&component_names, g_strfreev);

  // Remove an RPM component from a stream
  modulemd_module_stream_v1_remove_rpm_component (stream, "rpmcomponent");
  component_names =
    modulemd_module_stream_v1_get_rpm_component_names_as_strv (stream);
  g_assert_cmpint (g_strv_length (component_names), ==, 0);

  g_clear_pointer (&component_names, g_strfreev);

  // Remove a Module component from a stream
  modulemd_module_stream_v1_remove_module_component (stream,
                                                     "modulecomponent");
  component_names =
    modulemd_module_stream_v1_get_module_component_names_as_strv (stream);
  g_assert_cmpint (g_strv_length (component_names), ==, 0);

  g_clear_pointer (&component_names, g_strfreev);

  g_clear_object (&module_component);
  g_clear_object (&rpm_component);
  g_clear_object (&stream);
}


static void
module_stream_v2_test_components (ModuleStreamFixture *fixture,
                                  gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  g_autoptr (ModulemdComponentRpm) rpm_component = NULL;
  g_autoptr (ModulemdComponentModule) module_component = NULL;
  ModulemdComponent *retrieved_component = NULL;
  g_auto (GStrv) component_names = NULL;

  stream = modulemd_module_stream_v2_new (NULL, NULL);

  // Add a RPM component to a stream
  rpm_component = modulemd_component_rpm_new ("rpmcomponent");
  modulemd_module_stream_v2_add_component (stream,
                                           (ModulemdComponent *)rpm_component);
  component_names =
    modulemd_module_stream_v2_get_rpm_component_names_as_strv (stream);
  g_assert_true (
    g_strv_contains ((const gchar *const *)component_names, "rpmcomponent"));
  g_assert_cmpint (g_strv_length (component_names), ==, 1);

  retrieved_component =
    (ModulemdComponent *)modulemd_module_stream_v2_get_rpm_component (
      stream, "rpmcomponent");
  g_assert_nonnull (retrieved_component);
  g_assert_true (modulemd_component_equals (
    retrieved_component, (ModulemdComponent *)rpm_component));

  g_clear_pointer (&component_names, g_strfreev);

  // Add a Module component to a stream
  module_component = modulemd_component_module_new ("modulecomponent");
  modulemd_module_stream_v2_add_component (
    stream, (ModulemdComponent *)module_component);
  component_names =
    modulemd_module_stream_v2_get_module_component_names_as_strv (stream);
  g_assert_true (g_strv_contains ((const gchar *const *)component_names,
                                  "modulecomponent"));
  g_assert_cmpint (g_strv_length (component_names), ==, 1);

  retrieved_component =
    (ModulemdComponent *)modulemd_module_stream_v2_get_module_component (
      stream, "modulecomponent");
  g_assert_nonnull (retrieved_component);
  g_assert_true (modulemd_component_equals (
    retrieved_component, (ModulemdComponent *)module_component));

  g_clear_pointer (&component_names, g_strfreev);

  // Remove an RPM component from a stream
  modulemd_module_stream_v2_remove_rpm_component (stream, "rpmcomponent");
  component_names =
    modulemd_module_stream_v2_get_rpm_component_names_as_strv (stream);
  g_assert_cmpint (g_strv_length (component_names), ==, 0);

  g_clear_pointer (&component_names, g_strfreev);

  // Remove a Module component from a stream
  modulemd_module_stream_v2_remove_module_component (stream,
                                                     "modulecomponent");
  component_names =
    modulemd_module_stream_v2_get_module_component_names_as_strv (stream);
  g_assert_cmpint (g_strv_length (component_names), ==, 0);

  g_clear_pointer (&component_names, g_strfreev);

  g_clear_object (&module_component);
  g_clear_object (&rpm_component);
  g_clear_object (&stream);
}

static void
module_stream_test_copy (ModuleStreamFixture *fixture, gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autoptr (ModulemdModuleStream) copied_stream = NULL;
  guint64 version;

  for (version = MD_MODULESTREAM_VERSION_ONE;
       version <= MD_MODULESTREAM_VERSION_LATEST;
       version++)
    {
      /* Test copying with a stream name */
      stream = modulemd_module_stream_new (version, "foo", "latest");
      copied_stream = modulemd_module_stream_copy (stream, NULL, NULL);
      g_assert_nonnull (copied_stream);
      g_assert_true (MODULEMD_IS_MODULE_STREAM (copied_stream));
      g_assert_cmpstr (modulemd_module_stream_get_module_name (stream),
                       ==,
                       modulemd_module_stream_get_module_name (copied_stream));
      g_assert_cmpstr (modulemd_module_stream_get_stream_name (stream),
                       ==,
                       modulemd_module_stream_get_stream_name (copied_stream));
      g_clear_object (&stream);
      g_clear_object (&copied_stream);


      /* Test copying without a stream name */
      stream = modulemd_module_stream_new (version, "foo", NULL);
      copied_stream = modulemd_module_stream_copy (stream, NULL, NULL);
      g_assert_nonnull (copied_stream);
      g_assert_true (MODULEMD_IS_MODULE_STREAM (copied_stream));
      g_assert_cmpstr (modulemd_module_stream_get_module_name (stream),
                       ==,
                       modulemd_module_stream_get_module_name (copied_stream));
      g_assert_cmpstr (modulemd_module_stream_get_stream_name (stream),
                       ==,
                       modulemd_module_stream_get_stream_name (copied_stream));
      g_clear_object (&stream);
      g_clear_object (&copied_stream);

      /* Test copying with and renaming the stream name */
      stream = modulemd_module_stream_new (version, "foo", "latest");
      copied_stream = modulemd_module_stream_copy (stream, NULL, "earliest");
      g_assert_nonnull (copied_stream);
      g_assert_true (MODULEMD_IS_MODULE_STREAM (copied_stream));
      g_assert_cmpstr (modulemd_module_stream_get_module_name (stream),
                       ==,
                       modulemd_module_stream_get_module_name (copied_stream));
      g_assert_cmpstr (
        modulemd_module_stream_get_stream_name (stream), ==, "latest");
      g_assert_cmpstr (modulemd_module_stream_get_stream_name (copied_stream),
                       ==,
                       "earliest");
      g_clear_object (&stream);
      g_clear_object (&copied_stream);
    }
}


static void
module_stream_test_equals (ModuleStreamFixture *fixture,
                           gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream_1 = NULL;
  g_autoptr (ModulemdModuleStream) stream_2 = NULL;
  guint64 version;

  for (version = MD_MODULESTREAM_VERSION_ONE;
       version <= MD_MODULESTREAM_VERSION_LATEST;
       version++)
    {
      /* Test equality with same stream and module names */
      stream_1 = modulemd_module_stream_new (version, "foo", "latest");
      stream_2 = modulemd_module_stream_new (version, "foo", "latest");

      g_assert_true (modulemd_module_stream_equals (stream_1, stream_2));
      g_clear_object (&stream_1);
      g_clear_object (&stream_2);


      /* Test equality with different stream names*/
      stream_1 = modulemd_module_stream_new (version, "foo", NULL);
      stream_2 = modulemd_module_stream_new (version, "bar", NULL);

      g_assert_false (modulemd_module_stream_equals (stream_1, stream_2));
      g_clear_object (&stream_1);
      g_clear_object (&stream_2);

      /* Test equality with different module name */
      stream_1 = modulemd_module_stream_new (version, "bar", "thor");
      stream_2 = modulemd_module_stream_new (version, "bar", "loki");

      g_assert_false (modulemd_module_stream_equals (stream_1, stream_2));
      g_clear_object (&stream_1);
      g_clear_object (&stream_2);

      /* Test equality with same arch */
      stream_1 = modulemd_module_stream_new (version, "bar", "thor");
      modulemd_module_stream_set_arch (stream_1, "x86_64");
      stream_2 = modulemd_module_stream_new (version, "bar", "thor");
      modulemd_module_stream_set_arch (stream_2, "x86_64");

      g_assert_true (modulemd_module_stream_equals (stream_1, stream_2));
      g_clear_object (&stream_1);
      g_clear_object (&stream_2);

      /* Test equality with different arch */
      stream_1 = modulemd_module_stream_new (version, "bar", "thor");
      modulemd_module_stream_set_arch (stream_1, "x86_64");
      stream_2 = modulemd_module_stream_new (version, "bar", "thor");
      modulemd_module_stream_set_arch (stream_2, "x86_25");

      g_assert_false (modulemd_module_stream_equals (stream_1, stream_2));
      g_clear_object (&stream_1);
      g_clear_object (&stream_2);
    }
}


G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
module_stream_test_nsvc (ModuleStreamFixture *fixture, gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *s_nsvc = NULL;
  guint64 version;

  for (version = MD_MODULESTREAM_VERSION_ONE;
       version <= MD_MODULESTREAM_VERSION_LATEST;
       version++)
    {
      // First test that nsvc is None for a module with no name
      stream = modulemd_module_stream_new (version, NULL, NULL);
      s_nsvc = modulemd_module_stream_get_nsvc_as_string (stream);
      g_assert_null (s_nsvc);
      g_clear_pointer (&s_nsvc, g_free);
      g_clear_object (&stream);

      // Now with valid module and stream names
      stream = modulemd_module_stream_new (version, "modulename", NULL);
      s_nsvc = modulemd_module_stream_get_nsvc_as_string (stream);
      g_assert_null (s_nsvc);
      g_clear_pointer (&s_nsvc, g_free);
      g_clear_object (&stream);

      // Now with valid module and stream names
      stream =
        modulemd_module_stream_new (version, "modulename", "streamname");
      s_nsvc = modulemd_module_stream_get_nsvc_as_string (stream);
      g_assert_cmpstr (s_nsvc, ==, "modulename:streamname:0");
      g_clear_pointer (&s_nsvc, g_free);

      //# Add a version number
      modulemd_module_stream_set_version (stream, 42);
      s_nsvc = modulemd_module_stream_get_nsvc_as_string (stream);
      g_assert_cmpstr (s_nsvc, ==, "modulename:streamname:42");
      g_clear_pointer (&s_nsvc, g_free);

      // Add a context
      modulemd_module_stream_set_context (stream, "deadbeef");
      s_nsvc = modulemd_module_stream_get_nsvc_as_string (stream);
      g_assert_cmpstr (s_nsvc, ==, "modulename:streamname:42:deadbeef");
      g_clear_pointer (&s_nsvc, g_free);
      g_clear_object (&stream);
    }
}
G_GNUC_END_IGNORE_DEPRECATIONS


static void
module_stream_test_nsvca (ModuleStreamFixture *fixture,
                          gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *s_nsvca = NULL;
  guint64 version;

  for (version = MD_MODULESTREAM_VERSION_ONE;
       version <= MD_MODULESTREAM_VERSION_LATEST;
       version++)
    {
      // First test that NSVCA is None for a module with no name
      stream = modulemd_module_stream_new (version, NULL, NULL);
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_null (s_nsvca);
      g_clear_pointer (&s_nsvca, g_free);
      g_clear_object (&stream);

      // Now with valid module and stream names
      stream = modulemd_module_stream_new (version, "modulename", NULL);
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename");
      g_clear_pointer (&s_nsvca, g_free);
      g_clear_object (&stream);

      // Now with valid module and stream names
      stream =
        modulemd_module_stream_new (version, "modulename", "streamname");
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename:streamname");
      g_clear_pointer (&s_nsvca, g_free);

      //# Add a version number
      modulemd_module_stream_set_version (stream, 42);
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename:streamname:42");
      g_clear_pointer (&s_nsvca, g_free);

      // Add a context
      modulemd_module_stream_set_context (stream, "deadbeef");
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename:streamname:42:deadbeef");
      g_clear_pointer (&s_nsvca, g_free);

      // Add an architecture
      modulemd_module_stream_set_arch (stream, "x86_64");
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (
        s_nsvca, ==, "modulename:streamname:42:deadbeef:x86_64");
      g_clear_pointer (&s_nsvca, g_free);

      // Now try removing some of the bits in the middle
      modulemd_module_stream_set_context (stream, NULL);
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename:streamname:42::x86_64");
      g_clear_pointer (&s_nsvca, g_free);
      g_clear_object (&stream);

      stream = modulemd_module_stream_new (version, "modulename", NULL);
      modulemd_module_stream_set_arch (stream, "x86_64");
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename::::x86_64");
      g_clear_pointer (&s_nsvca, g_free);

      modulemd_module_stream_set_version (stream, 2019);
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename::2019::x86_64");
      g_clear_pointer (&s_nsvca, g_free);

      // Add a context
      modulemd_module_stream_set_context (stream, "feedfeed");
      s_nsvca = modulemd_module_stream_get_NSVCA_as_string (stream);
      g_assert_cmpstr (s_nsvca, ==, "modulename::2019:feedfeed:x86_64");
      g_clear_pointer (&s_nsvca, g_free);
      g_clear_object (&stream);
    }
}


static void
module_stream_v1_test_equals (ModuleStreamFixture *fixture,
                              gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV1) stream_1 = NULL;
  g_autoptr (ModulemdModuleStreamV1) stream_2 = NULL;
  g_autoptr (ModulemdProfile) profile_1 = NULL;
  g_autoptr (ModulemdServiceLevel) servicelevel_1 = NULL;
  g_autoptr (ModulemdServiceLevel) servicelevel_2 = NULL;
  g_autoptr (ModulemdComponentModule) component_1 = NULL;
  g_autoptr (ModulemdComponentRpm) component_2 = NULL;

  /*Test equality of 2 streams with same string constants*/
  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_set_community (stream_1, "community_1");
  modulemd_module_stream_v1_set_description (stream_1, "description_1");
  modulemd_module_stream_v1_set_documentation (stream_1, "documentation_1");
  modulemd_module_stream_v1_set_summary (stream_1, "summary_1");
  modulemd_module_stream_v1_set_tracker (stream_1, "tracker_1");

  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_set_community (stream_2, "community_1");
  modulemd_module_stream_v1_set_description (stream_2, "description_1");
  modulemd_module_stream_v1_set_documentation (stream_2, "documentation_1");
  modulemd_module_stream_v1_set_summary (stream_2, "summary_1");
  modulemd_module_stream_v1_set_tracker (stream_2, "tracker_1");

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with certain different string constants*/
  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_set_community (stream_1, "community_1");
  modulemd_module_stream_v1_set_description (stream_1, "description_1");
  modulemd_module_stream_v1_set_documentation (stream_1, "documentation_1");
  modulemd_module_stream_v1_set_summary (stream_1, "summary_1");
  modulemd_module_stream_v1_set_tracker (stream_1, "tracker_1");

  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_set_community (stream_2, "community_1");
  modulemd_module_stream_v1_set_description (stream_2, "description_2");
  modulemd_module_stream_v1_set_documentation (stream_2, "documentation_1");
  modulemd_module_stream_v1_set_summary (stream_2, "summary_2");
  modulemd_module_stream_v1_set_tracker (stream_2, "tracker_2");

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with same hashtable sets*/
  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_rpm_api (stream_1, "rpm_1");
  modulemd_module_stream_v1_add_rpm_api (stream_1, "rpm_2");
  modulemd_module_stream_v1_add_module_license (stream_1, "module_a");
  modulemd_module_stream_v1_add_module_license (stream_1, "module_b");
  modulemd_module_stream_v1_add_content_license (stream_1, "content_a");
  modulemd_module_stream_v1_add_content_license (stream_1, "content_b");
  modulemd_module_stream_v1_add_rpm_artifact (stream_1, "artifact_a");
  modulemd_module_stream_v1_add_rpm_artifact (stream_1, "artifact_b");
  modulemd_module_stream_v1_add_rpm_filter (stream_1, "filter_a");
  modulemd_module_stream_v1_add_rpm_filter (stream_1, "filter_b");

  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_rpm_api (stream_2, "rpm_1");
  modulemd_module_stream_v1_add_rpm_api (stream_2, "rpm_2");
  modulemd_module_stream_v1_add_module_license (stream_2, "module_a");
  modulemd_module_stream_v1_add_module_license (stream_2, "module_b");
  modulemd_module_stream_v1_add_content_license (stream_2, "content_a");
  modulemd_module_stream_v1_add_content_license (stream_2, "content_b");
  modulemd_module_stream_v1_add_rpm_artifact (stream_2, "artifact_a");
  modulemd_module_stream_v1_add_rpm_artifact (stream_2, "artifact_b");
  modulemd_module_stream_v1_add_rpm_filter (stream_2, "filter_a");
  modulemd_module_stream_v1_add_rpm_filter (stream_2, "filter_b");

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with different hashtable sets*/
  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_rpm_api (stream_1, "rpm_1");
  modulemd_module_stream_v1_add_rpm_api (stream_1, "rpm_2");
  modulemd_module_stream_v1_add_module_license (stream_1, "module_a");
  modulemd_module_stream_v1_add_module_license (stream_1, "module_b");
  modulemd_module_stream_v1_add_content_license (stream_1, "content_a");
  modulemd_module_stream_v1_add_content_license (stream_1, "content_b");
  modulemd_module_stream_v1_add_rpm_artifact (stream_1, "artifact_a");
  modulemd_module_stream_v1_add_rpm_artifact (stream_1, "artifact_b");
  modulemd_module_stream_v1_add_rpm_artifact (stream_1, "artifact_c");
  modulemd_module_stream_v1_add_rpm_filter (stream_1, "filter_a");
  modulemd_module_stream_v1_add_rpm_filter (stream_1, "filter_b");

  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_rpm_api (stream_2, "rpm_1");
  modulemd_module_stream_v1_add_module_license (stream_2, "module_a");
  modulemd_module_stream_v1_add_module_license (stream_2, "module_b");
  modulemd_module_stream_v1_add_content_license (stream_2, "content_a");
  modulemd_module_stream_v1_add_content_license (stream_2, "content_b");
  modulemd_module_stream_v1_add_rpm_artifact (stream_2, "artifact_a");
  modulemd_module_stream_v1_add_rpm_artifact (stream_2, "artifact_b");
  modulemd_module_stream_v1_add_rpm_filter (stream_2, "filter_a");
  modulemd_module_stream_v1_add_rpm_filter (stream_2, "filter_b");

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with same dependencies*/
  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_buildtime_requirement (
    stream_1, "testmodule", "stable");
  modulemd_module_stream_v1_add_runtime_requirement (
    stream_1, "testmodule", "latest");
  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_buildtime_requirement (
    stream_2, "testmodule", "stable");
  modulemd_module_stream_v1_add_runtime_requirement (
    stream_2, "testmodule", "latest");

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with different dependencies*/
  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_buildtime_requirement (
    stream_1, "test", "stable");
  modulemd_module_stream_v1_add_runtime_requirement (
    stream_1, "testmodule", "latest");
  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_buildtime_requirement (
    stream_2, "testmodule", "stable");
  modulemd_module_stream_v1_add_runtime_requirement (
    stream_2, "testmodule", "not_latest");

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with same hashtables*/
  profile_1 = modulemd_profile_new ("testprofile");
  component_1 = modulemd_component_module_new ("testmodule");
  servicelevel_1 = modulemd_service_level_new ("foo");

  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_profile (stream_1, profile_1);
  modulemd_module_stream_v1_add_component (stream_1,
                                           (ModulemdComponent *)component_1);
  modulemd_module_stream_v1_add_servicelevel (stream_1, servicelevel_1);
  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_profile (stream_2, profile_1);
  modulemd_module_stream_v1_add_component (stream_2,
                                           (ModulemdComponent *)component_1);
  modulemd_module_stream_v1_add_servicelevel (stream_2, servicelevel_1);

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&profile_1);
  g_clear_object (&component_1);
  g_clear_object (&servicelevel_1);

  /*Test equality of 2 streams with different hashtables*/
  profile_1 = modulemd_profile_new ("testprofile");
  component_1 = modulemd_component_module_new ("testmodule");
  component_2 = modulemd_component_rpm_new ("something");
  servicelevel_1 = modulemd_service_level_new ("foo");
  servicelevel_2 = modulemd_service_level_new ("bar");

  stream_1 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_profile (stream_1, profile_1);
  modulemd_module_stream_v1_add_component (stream_1,
                                           (ModulemdComponent *)component_1);
  modulemd_module_stream_v1_add_servicelevel (stream_1, servicelevel_1);
  stream_2 = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_profile (stream_2, profile_1);
  modulemd_module_stream_v1_add_component (stream_2,
                                           (ModulemdComponent *)component_2);
  modulemd_module_stream_v1_add_servicelevel (stream_2, servicelevel_2);

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&profile_1);
  g_clear_object (&component_1);
  g_clear_object (&component_2);
  g_clear_object (&servicelevel_1);
  g_clear_object (&servicelevel_2);
}


static void
module_stream_v2_test_equals (ModuleStreamFixture *fixture,
                              gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream_1 = NULL;
  g_autoptr (ModulemdModuleStreamV2) stream_2 = NULL;
  g_autoptr (ModulemdProfile) profile_1 = NULL;
  g_autoptr (ModulemdServiceLevel) servicelevel_1 = NULL;
  g_autoptr (ModulemdServiceLevel) servicelevel_2 = NULL;
  g_autoptr (ModulemdComponentModule) component_1 = NULL;
  g_autoptr (ModulemdComponentRpm) component_2 = NULL;
  g_autoptr (ModulemdDependencies) dep_1 = NULL;
  g_autoptr (ModulemdDependencies) dep_2 = NULL;
  g_autoptr (ModulemdRpmMapEntry) entry_1 = NULL;

  /*Test equality of 2 streams with same string constants*/
  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_community (stream_1, "community_1");
  modulemd_module_stream_v2_set_description (stream_1, "description_1");
  modulemd_module_stream_v2_set_documentation (stream_1, "documentation_1");
  modulemd_module_stream_v2_set_summary (stream_1, "summary_1");
  modulemd_module_stream_v2_set_tracker (stream_1, "tracker_1");

  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_community (stream_2, "community_1");
  modulemd_module_stream_v2_set_description (stream_2, "description_1");
  modulemd_module_stream_v2_set_documentation (stream_2, "documentation_1");
  modulemd_module_stream_v2_set_summary (stream_2, "summary_1");
  modulemd_module_stream_v2_set_tracker (stream_2, "tracker_1");

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with certain different string constants*/
  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_community (stream_1, "community_1");
  modulemd_module_stream_v2_set_description (stream_1, "description_1");
  modulemd_module_stream_v2_set_documentation (stream_1, "documentation_1");
  modulemd_module_stream_v2_set_summary (stream_1, "summary_1");
  modulemd_module_stream_v2_set_tracker (stream_1, "tracker_1");

  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_community (stream_2, "community_1");
  modulemd_module_stream_v2_set_description (stream_2, "description_2");
  modulemd_module_stream_v2_set_documentation (stream_2, "documentation_1");
  modulemd_module_stream_v2_set_summary (stream_2, "summary_2");
  modulemd_module_stream_v2_set_tracker (stream_2, "tracker_2");

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with same hashtable sets*/
  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_rpm_api (stream_1, "rpm_1");
  modulemd_module_stream_v2_add_rpm_api (stream_1, "rpm_2");
  modulemd_module_stream_v2_add_module_license (stream_1, "module_a");
  modulemd_module_stream_v2_add_module_license (stream_1, "module_b");
  modulemd_module_stream_v2_add_content_license (stream_1, "content_a");
  modulemd_module_stream_v2_add_content_license (stream_1, "content_b");
  modulemd_module_stream_v2_add_rpm_artifact (stream_1, "artifact_a");
  modulemd_module_stream_v2_add_rpm_artifact (stream_1, "artifact_b");
  modulemd_module_stream_v2_add_rpm_filter (stream_1, "filter_a");
  modulemd_module_stream_v2_add_rpm_filter (stream_1, "filter_b");

  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_rpm_api (stream_2, "rpm_1");
  modulemd_module_stream_v2_add_rpm_api (stream_2, "rpm_2");
  modulemd_module_stream_v2_add_module_license (stream_2, "module_a");
  modulemd_module_stream_v2_add_module_license (stream_2, "module_b");
  modulemd_module_stream_v2_add_content_license (stream_2, "content_a");
  modulemd_module_stream_v2_add_content_license (stream_2, "content_b");
  modulemd_module_stream_v2_add_rpm_artifact (stream_2, "artifact_a");
  modulemd_module_stream_v2_add_rpm_artifact (stream_2, "artifact_b");
  modulemd_module_stream_v2_add_rpm_filter (stream_2, "filter_a");
  modulemd_module_stream_v2_add_rpm_filter (stream_2, "filter_b");

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with different hashtable sets*/
  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_rpm_api (stream_1, "rpm_1");
  modulemd_module_stream_v2_add_rpm_api (stream_1, "rpm_2");
  modulemd_module_stream_v2_add_module_license (stream_1, "module_a");
  modulemd_module_stream_v2_add_module_license (stream_1, "module_b");
  modulemd_module_stream_v2_add_content_license (stream_1, "content_a");
  modulemd_module_stream_v2_add_content_license (stream_1, "content_b");
  modulemd_module_stream_v2_add_rpm_artifact (stream_1, "artifact_a");
  modulemd_module_stream_v2_add_rpm_artifact (stream_1, "artifact_b");
  modulemd_module_stream_v2_add_rpm_artifact (stream_1, "artifact_c");
  modulemd_module_stream_v2_add_rpm_filter (stream_1, "filter_a");
  modulemd_module_stream_v2_add_rpm_filter (stream_1, "filter_b");

  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_rpm_api (stream_2, "rpm_1");
  modulemd_module_stream_v2_add_module_license (stream_2, "module_a");
  modulemd_module_stream_v2_add_module_license (stream_2, "module_b");
  modulemd_module_stream_v2_add_content_license (stream_2, "content_a");
  modulemd_module_stream_v2_add_content_license (stream_2, "content_b");
  modulemd_module_stream_v2_add_rpm_artifact (stream_2, "artifact_a");
  modulemd_module_stream_v2_add_rpm_artifact (stream_2, "artifact_b");
  modulemd_module_stream_v2_add_rpm_filter (stream_2, "filter_a");
  modulemd_module_stream_v2_add_rpm_filter (stream_2, "filter_b");

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);

  /*Test equality of 2 streams with same hashtables*/
  profile_1 = modulemd_profile_new ("testprofile");
  component_1 = modulemd_component_module_new ("testmodule");
  servicelevel_1 = modulemd_service_level_new ("foo");

  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_profile (stream_1, profile_1);
  modulemd_module_stream_v2_add_component (stream_1,
                                           (ModulemdComponent *)component_1);
  modulemd_module_stream_v2_add_servicelevel (stream_1, servicelevel_1);
  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_profile (stream_2, profile_1);
  modulemd_module_stream_v2_add_component (stream_2,
                                           (ModulemdComponent *)component_1);
  modulemd_module_stream_v2_add_servicelevel (stream_2, servicelevel_1);

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&profile_1);
  g_clear_object (&component_1);
  g_clear_object (&servicelevel_1);

  /*Test equality of 2 streams with different hashtables*/
  profile_1 = modulemd_profile_new ("testprofile");
  component_1 = modulemd_component_module_new ("testmodule");
  component_2 = modulemd_component_rpm_new ("something");
  servicelevel_1 = modulemd_service_level_new ("foo");
  servicelevel_2 = modulemd_service_level_new ("bar");

  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_profile (stream_1, profile_1);
  modulemd_module_stream_v2_add_component (stream_1,
                                           (ModulemdComponent *)component_1);
  modulemd_module_stream_v2_add_servicelevel (stream_1, servicelevel_1);
  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_profile (stream_2, profile_1);
  modulemd_module_stream_v2_add_component (stream_2,
                                           (ModulemdComponent *)component_2);
  modulemd_module_stream_v2_add_servicelevel (stream_2, servicelevel_2);

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&profile_1);
  g_clear_object (&component_1);
  g_clear_object (&component_2);
  g_clear_object (&servicelevel_1);
  g_clear_object (&servicelevel_2);

  /*Test equality of 2 streams with same dependencies*/
  dep_1 = modulemd_dependencies_new ();
  modulemd_dependencies_add_buildtime_stream (dep_1, "foo", "stable");

  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_dependencies (stream_1, dep_1);
  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_dependencies (stream_2, dep_1);

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&dep_1);

  /*Test equality of 2 streams with different dependencies*/
  dep_1 = modulemd_dependencies_new ();
  modulemd_dependencies_add_buildtime_stream (dep_1, "foo", "stable");
  dep_2 = modulemd_dependencies_new ();
  modulemd_dependencies_add_buildtime_stream (dep_2, "foo", "latest");

  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_dependencies (stream_1, dep_1);
  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_add_dependencies (stream_2, dep_2);

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&dep_1);
  g_clear_object (&dep_2);

  /*Test equality of 2 streams with same rpm artifact map entry*/
  entry_1 = modulemd_rpm_map_entry_new (
    "bar", 0, "1.23", "1.module_deadbeef", "x86_64");

  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_rpm_artifact_map_entry (
    stream_1, entry_1, "sha256", "baddad");
  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_rpm_artifact_map_entry (
    stream_2, entry_1, "sha256", "baddad");

  g_assert_true (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&entry_1);

  /*Test equality of 2 streams with different rpm artifact map entry*/
  entry_1 = modulemd_rpm_map_entry_new (
    "bar", 0, "1.23", "1.module_deadbeef", "x86_64");

  stream_1 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_rpm_artifact_map_entry (
    stream_1, entry_1, "sha256", "baddad");
  stream_2 = modulemd_module_stream_v2_new (NULL, NULL);
  modulemd_module_stream_v2_set_rpm_artifact_map_entry (
    stream_2, entry_1, "sha256", "badmom");

  g_assert_false (modulemd_module_stream_equals (
    (ModulemdModuleStream *)stream_1, (ModulemdModuleStream *)stream_2));
  g_clear_object (&stream_1);
  g_clear_object (&stream_2);
  g_clear_object (&entry_1);
}


static void
module_stream_v1_test_dependencies (ModuleStreamFixture *fixture,
                                    gconstpointer user_data)
{
  g_auto (GStrv) list = NULL;
  g_autoptr (ModulemdModuleStreamV1) stream = NULL;
  stream = modulemd_module_stream_v1_new (NULL, NULL);
  modulemd_module_stream_v1_add_buildtime_requirement (
    stream, "testmodule", "stable");
  list = modulemd_module_stream_v1_get_buildtime_modules_as_strv (stream);
  g_assert_cmpint (g_strv_length (list), ==, 1);
  g_assert_cmpstr (list[0], ==, "testmodule");
  g_assert_cmpstr (modulemd_module_stream_v1_get_buildtime_requirement_stream (
                     stream, "testmodule"),
                   ==,
                   "stable");
  g_clear_pointer (&list, g_strfreev);

  modulemd_module_stream_v1_add_runtime_requirement (
    stream, "testmodule", "latest");
  list = modulemd_module_stream_v1_get_runtime_modules_as_strv (stream);
  g_assert_cmpint (g_strv_length (list), ==, 1);
  g_assert_cmpstr (list[0], ==, "testmodule");
  g_assert_cmpstr (modulemd_module_stream_v1_get_runtime_requirement_stream (
                     stream, "testmodule"),
                   ==,
                   "latest");
  g_clear_pointer (&list, g_strfreev);
  g_clear_object (&stream);
}


static void
module_stream_v2_test_dependencies (ModuleStreamFixture *fixture,
                                    gconstpointer user_data)
{
  g_auto (GStrv) list = NULL;
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  g_autoptr (ModulemdDependencies) dep = NULL;
  stream = modulemd_module_stream_v2_new (NULL, NULL);
  dep = modulemd_dependencies_new ();
  modulemd_dependencies_add_buildtime_stream (dep, "foo", "stable");
  modulemd_dependencies_set_empty_runtime_dependencies_for_module (dep, "bar");
  modulemd_module_stream_v2_add_dependencies (stream, dep);
  GPtrArray *deps_list = modulemd_module_stream_v2_get_dependencies (stream);
  g_assert_cmpint (deps_list->len, ==, 1);

  list = modulemd_dependencies_get_buildtime_modules_as_strv (
    g_ptr_array_index (deps_list, 0));
  g_assert_cmpstr (list[0], ==, "foo");
  g_clear_pointer (&list, g_strfreev);

  list = modulemd_dependencies_get_buildtime_streams_as_strv (
    g_ptr_array_index (deps_list, 0), "foo");
  g_assert_nonnull (list);
  g_assert_cmpstr (list[0], ==, "stable");
  g_assert_null (list[1]);
  g_clear_pointer (&list, g_strfreev);

  list = modulemd_dependencies_get_runtime_modules_as_strv (
    g_ptr_array_index (deps_list, 0));
  g_assert_nonnull (list);
  g_assert_cmpstr (list[0], ==, "bar");
  g_assert_null (list[1]);

  g_clear_pointer (&list, g_strfreev);
  g_clear_object (&dep);
  g_clear_object (&stream);
}


static void
module_stream_v1_test_parse_dump (ModuleStreamFixture *fixture,
                                  gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV1) stream = NULL;
  g_autoptr (GError) error = NULL;
  gboolean ret;
  MMD_INIT_YAML_PARSER (parser);
  MMD_INIT_YAML_EVENT (event);
  MMD_INIT_YAML_EMITTER (emitter);
  MMD_INIT_YAML_STRING (&emitter, yaml_string);
  g_autofree gchar *yaml_path = NULL;
  g_autoptr (FILE) yaml_stream = NULL;
  g_autoptr (ModulemdSubdocumentInfo) subdoc = NULL;
  yaml_path =
    g_strdup_printf ("%s/spec.v1.yaml", g_getenv ("MESON_SOURCE_ROOT"));
  g_assert_nonnull (yaml_path);

  yaml_stream = g_fopen (yaml_path, "rbe");
  g_assert_nonnull (yaml_stream);

  /* First parse it */
  yaml_parser_set_input_file (&parser, yaml_stream);
  g_assert_true (yaml_parser_parse (&parser, &event));
  g_assert_cmpint (event.type, ==, YAML_STREAM_START_EVENT);
  yaml_event_delete (&event);
  g_assert_true (yaml_parser_parse (&parser, &event));
  g_assert_cmpint (event.type, ==, YAML_DOCUMENT_START_EVENT);
  yaml_event_delete (&event);

  subdoc = modulemd_yaml_parse_document_type (&parser);
  g_assert_nonnull (subdoc);
  g_assert_null (modulemd_subdocument_info_get_gerror (subdoc));

  g_assert_cmpint (modulemd_subdocument_info_get_doctype (subdoc),
                   ==,
                   MODULEMD_YAML_DOC_MODULESTREAM);
  g_assert_cmpint (modulemd_subdocument_info_get_mdversion (subdoc), ==, 1);
  g_assert_nonnull (modulemd_subdocument_info_get_yaml (subdoc));

  stream = modulemd_module_stream_v1_parse_yaml (subdoc, TRUE, &error);
  g_assert_no_error (error);
  g_assert_nonnull (stream);

  /* Then dump it */
  g_debug ("Starting dumping");
  g_assert_true (mmd_emitter_start_stream (&emitter, &error));
  ret = modulemd_module_stream_v1_emit_yaml (stream, &emitter, &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  ret = mmd_emitter_end_stream (&emitter, &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  g_assert_nonnull (yaml_string->str);

  g_assert_cmpstr (
    yaml_string->str,
    ==,
    "---\n"
    "document: modulemd\n"
    "version: 1\n"
    "data:\n"
    "  name: foo\n"
    "  stream: stream-name\n"
    "  version: 20160927144203\n"
    "  context: c0ffee43\n"
    "  arch: x86_64\n"
    "  summary: An example module\n"
    "  description: >-\n"
    "    A module for the demonstration of the metadata format. Also, the "
    "obligatory lorem\n"
    "    ipsum dolor sit amet goes right here.\n"
    "  servicelevels:\n"
    "    bug_fixes:\n"
    "      eol: 2077-10-23\n"
    "    rawhide:\n"
    "      eol: 2077-10-23\n"
    "    security_fixes:\n"
    "      eol: 2077-10-23\n"
    "    stable_api:\n"
    "      eol: 2077-10-23\n"
    "  license:\n"
    "    module:\n"
    "    - MIT\n"
    "    content:\n"
    "    - Beerware\n"
    "    - GPLv2+\n"
    "    - zlib\n"
    "  xmd:\n"
    "    some_key: some_data\n"
    "  dependencies:\n"
    "    buildrequires:\n"
    "      extra-build-env: and-its-stream-name-too\n"
    "      platform: and-its-stream-name\n"
    "    requires:\n"
    "      platform: and-its-stream-name\n"
    "  references:\n"
    "    community: http://www.example.com/\n"
    "    documentation: http://www.example.com/\n"
    "    tracker: http://www.example.com/\n"
    "  profiles:\n"
    "    buildroot:\n"
    "      rpms:\n"
    "      - bar-devel\n"
    "    container:\n"
    "      rpms:\n"
    "      - bar\n"
    "      - bar-devel\n"
    "    default:\n"
    "      rpms:\n"
    "      - bar\n"
    "      - bar-extras\n"
    "      - baz\n"
    "    minimal:\n"
    "      description: Minimal profile installing only the bar package.\n"
    "      rpms:\n"
    "      - bar\n"
    "    srpm-buildroot:\n"
    "      rpms:\n"
    "      - bar-extras\n"
    "  api:\n"
    "    rpms:\n"
    "    - bar\n"
    "    - bar-devel\n"
    "    - bar-extras\n"
    "    - baz\n"
    "    - xxx\n"
    "  filter:\n"
    "    rpms:\n"
    "    - baz-nonfoo\n"
    "  buildopts:\n"
    "    rpms:\n"
    "      macros: >\n"
    "        %demomacro 1\n"
    "\n"
    "        %demomacro2 %{demomacro}23\n"
    "  components:\n"
    "    rpms:\n"
    "      bar:\n"
    "        rationale: We need this to demonstrate stuff.\n"
    "        repository: https://pagure.io/bar.git\n"
    "        cache: https://example.com/cache\n"
    "        ref: 26ca0c0\n"
    "      baz:\n"
    "        rationale: This one is here to demonstrate other stuff.\n"
    "      xxx:\n"
    "        rationale: xxx demonstrates arches and multilib.\n"
    "        arches: [i686, x86_64]\n"
    "        multilib: [x86_64]\n"
    "      xyz:\n"
    "        rationale: xyz is a bundled dependency of xxx.\n"
    "        buildorder: 10\n"
    "    modules:\n"
    "      includedmodule:\n"
    "        rationale: Included in the stack, just because.\n"
    "        repository: https://pagure.io/includedmodule.git\n"
    "        ref: somecoolbranchname\n"
    "        buildorder: 100\n"
    "  artifacts:\n"
    "    rpms:\n"
    "    - bar-0:1.23-1.module_deadbeef.x86_64\n"
    "    - bar-devel-0:1.23-1.module_deadbeef.x86_64\n"
    "    - bar-extras-0:1.23-1.module_deadbeef.x86_64\n"
    "    - baz-0:42-42.module_deadbeef.x86_64\n"
    "    - xxx-0:1-1.module_deadbeef.i686\n"
    "    - xxx-0:1-1.module_deadbeef.x86_64\n"
    "    - xyz-0:1-1.module_deadbeef.x86_64\n"
    "...\n");
}

static void
module_stream_v2_test_parse_dump (ModuleStreamFixture *fixture,
                                  gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  g_autoptr (GError) error = NULL;
  gboolean ret;
  MMD_INIT_YAML_PARSER (parser);
  MMD_INIT_YAML_EVENT (event);
  MMD_INIT_YAML_EMITTER (emitter);
  MMD_INIT_YAML_STRING (&emitter, yaml_string);
  g_autofree gchar *yaml_path = NULL;
  g_autoptr (FILE) yaml_stream = NULL;
  g_autoptr (ModulemdSubdocumentInfo) subdoc = NULL;
  yaml_path =
    g_strdup_printf ("%s/spec.v2.yaml", g_getenv ("MESON_SOURCE_ROOT"));
  g_assert_nonnull (yaml_path);

  yaml_stream = g_fopen (yaml_path, "rbe");
  g_assert_nonnull (yaml_stream);

  /* First parse it */
  yaml_parser_set_input_file (&parser, yaml_stream);
  g_assert_true (yaml_parser_parse (&parser, &event));
  g_assert_cmpint (event.type, ==, YAML_STREAM_START_EVENT);
  yaml_event_delete (&event);
  g_assert_true (yaml_parser_parse (&parser, &event));
  g_assert_cmpint (event.type, ==, YAML_DOCUMENT_START_EVENT);
  yaml_event_delete (&event);

  subdoc = modulemd_yaml_parse_document_type (&parser);
  g_assert_nonnull (subdoc);
  g_assert_null (modulemd_subdocument_info_get_gerror (subdoc));

  g_assert_cmpint (modulemd_subdocument_info_get_doctype (subdoc),
                   ==,
                   MODULEMD_YAML_DOC_MODULESTREAM);
  g_assert_cmpint (modulemd_subdocument_info_get_mdversion (subdoc), ==, 2);
  g_assert_nonnull (modulemd_subdocument_info_get_yaml (subdoc));

  stream = modulemd_module_stream_v2_parse_yaml (subdoc, TRUE, &error);
  g_assert_no_error (error);
  g_assert_nonnull (stream);

  /* Then dump it */
  g_debug ("Starting dumping");
  g_assert_true (mmd_emitter_start_stream (&emitter, &error));
  ret = modulemd_module_stream_v2_emit_yaml (stream, &emitter, &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  ret = mmd_emitter_end_stream (&emitter, &error);
  g_assert_no_error (error);
  g_assert_true (ret);
  g_assert_nonnull (yaml_string->str);

  g_assert_cmpstr (
    yaml_string->str,
    ==,
    "---\n"
    "document: modulemd\n"
    "version: 2\n"
    "data:\n"
    "  name: foo\n"
    "  stream: latest\n"
    "  version: 20160927144203\n"
    "  context: c0ffee43\n"
    "  arch: x86_64\n"
    "  summary: An example module\n"
    "  description: >-\n"
    "    A module for the demonstration of the metadata format. Also, the "
    "obligatory lorem\n"
    "    ipsum dolor sit amet goes right here.\n"
    "  servicelevels:\n"
    "    bug_fixes:\n"
    "      eol: 2077-10-23\n"
    "    rawhide:\n"
    "      eol: 2077-10-23\n"
    "    security_fixes:\n"
    "      eol: 2077-10-23\n"
    "    stable_api:\n"
    "      eol: 2077-10-23\n"
    "  license:\n"
    "    module:\n"
    "    - MIT\n"
    "    content:\n"
    "    - Beerware\n"
    "    - GPLv2+\n"
    "    - zlib\n"
    "  xmd:\n"
    "    some_key: some_data\n"
    "  dependencies:\n"
    "  - buildrequires:\n"
    "      platform: [-epel7, -f27, -f28]\n"
    "    requires:\n"
    "      platform: [-epel7, -f27, -f28]\n"
    "  - buildrequires:\n"
    "      buildtools: [v1, v2]\n"
    "      compatible: [v3]\n"
    "      platform: [f27]\n"
    "    requires:\n"
    "      compatible: [v3, v4]\n"
    "      platform: [f27]\n"
    "  - buildrequires:\n"
    "      platform: [f28]\n"
    "    requires:\n"
    "      platform: [f28]\n"
    "      runtime: [a, b]\n"
    "  - buildrequires:\n"
    "      extras: []\n"
    "      moreextras: [bar, foo]\n"
    "      platform: [epel7]\n"
    "    requires:\n"
    "      extras: []\n"
    "      moreextras: [bar, foo]\n"
    "      platform: [epel7]\n"
    "  references:\n"
    "    community: http://www.example.com/\n"
    "    documentation: http://www.example.com/\n"
    "    tracker: http://www.example.com/\n"
    "  profiles:\n"
    "    buildroot:\n"
    "      rpms:\n"
    "      - bar-devel\n"
    "    container:\n"
    "      rpms:\n"
    "      - bar\n"
    "      - bar-devel\n"
    "    default:\n"
    "      rpms:\n"
    "      - bar\n"
    "      - bar-extras\n"
    "      - baz\n"
    "    minimal:\n"
    "      description: Minimal profile installing only the bar package.\n"
    "      rpms:\n"
    "      - bar\n"
    "    srpm-buildroot:\n"
    "      rpms:\n"
    "      - bar-extras\n"
    "  api:\n"
    "    rpms:\n"
    "    - bar\n"
    "    - bar-devel\n"
    "    - bar-extras\n"
    "    - baz\n"
    "    - xxx\n"
    "  filter:\n"
    "    rpms:\n"
    "    - baz-nonfoo\n"
    "  buildopts:\n"
    "    rpms:\n"
    "      macros: >\n"
    "        %demomacro 1\n"
    "\n"
    "        %demomacro2 %{demomacro}23\n"
    "      whitelist:\n"
    "      - fooscl-1-bar\n"
    "      - fooscl-1-baz\n"
    "      - xxx\n"
    "      - xyz\n"
    "    arches: [i686, x86_64]\n"
    "  components:\n"
    "    rpms:\n"
    "      bar:\n"
    "        rationale: We need this to demonstrate stuff.\n"
    "        name: bar-real\n"
    "        repository: https://pagure.io/bar.git\n"
    "        cache: https://example.com/cache\n"
    "        ref: 26ca0c0\n"
    "      baz:\n"
    "        rationale: This one is here to demonstrate other stuff.\n"
    "      xxx:\n"
    "        rationale: xxx demonstrates arches and multilib.\n"
    "        arches: [i686, x86_64]\n"
    "        multilib: [x86_64]\n"
    "      xyz:\n"
    "        rationale: xyz is a bundled dependency of xxx.\n"
    "        buildorder: 10\n"
    "    modules:\n"
    "      includedmodule:\n"
    "        rationale: Included in the stack, just because.\n"
    "        repository: https://pagure.io/includedmodule.git\n"
    "        ref: somecoolbranchname\n"
    "        buildorder: 100\n"
    "  artifacts:\n"
    "    rpms:\n"
    "    - bar-0:1.23-1.module_deadbeef.x86_64\n"
    "    - bar-devel-0:1.23-1.module_deadbeef.x86_64\n"
    "    - bar-extras-0:1.23-1.module_deadbeef.x86_64\n"
    "    - baz-0:42-42.module_deadbeef.x86_64\n"
    "    - xxx-0:1-1.module_deadbeef.i686\n"
    "    - xxx-0:1-1.module_deadbeef.x86_64\n"
    "    - xyz-0:1-1.module_deadbeef.x86_64\n"
    "    rpm-map:\n"
    "      sha256:\n"
    "        "
    "ee47083ed80146eb2c84e9a94d0836393912185dcda62b9d93ee0c2ea5dc795b:\n"
    "          name: bar\n"
    "          epoch: 0\n"
    "          version: 1.23\n"
    "          release: 1.module_deadbeef\n"
    "          arch: x86_64\n"
    "          nevra: bar-0:1.23-1.module_deadbeef.x86_64\n"
    "...\n");
}

static void
module_stream_v1_test_depends_on_stream (ModuleStreamFixture *fixture,
                                         gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr (GError) error = NULL;
  g_autofree gchar *module_name = NULL;
  g_autofree gchar *module_stream = NULL;

  path = g_strdup_printf ("%s/dependson_v1.yaml", g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (
    path, TRUE, module_name, module_stream, &error);
  g_assert_nonnull (stream);

  g_assert_true (
    modulemd_module_stream_depends_on_stream (stream, "platform", "f30"));
  g_assert_true (modulemd_module_stream_build_depends_on_stream (
    stream, "platform", "f30"));

  g_assert_false (
    modulemd_module_stream_depends_on_stream (stream, "platform", "f28"));
  g_assert_false (modulemd_module_stream_build_depends_on_stream (
    stream, "platform", "f28"));

  g_assert_false (
    modulemd_module_stream_depends_on_stream (stream, "base", "f30"));
  g_assert_false (
    modulemd_module_stream_build_depends_on_stream (stream, "base", "f30"));
  g_clear_object (&stream);
}

static void
module_stream_v2_test_depends_on_stream (ModuleStreamFixture *fixture,
                                         gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr (GError) error = NULL;
  g_autofree gchar *module_name = NULL;
  g_autofree gchar *module_stream = NULL;

  path = g_strdup_printf ("%s/dependson_v2.yaml", g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (
    path, TRUE, module_name, module_stream, &error);
  g_assert_nonnull (stream);

  g_assert_true (
    modulemd_module_stream_depends_on_stream (stream, "platform", "f30"));
  g_assert_true (modulemd_module_stream_build_depends_on_stream (
    stream, "platform", "f30"));

  g_assert_false (
    modulemd_module_stream_depends_on_stream (stream, "platform", "f28"));
  g_assert_false (modulemd_module_stream_build_depends_on_stream (
    stream, "platform", "f28"));

  g_assert_false (
    modulemd_module_stream_depends_on_stream (stream, "base", "f30"));
  g_assert_false (
    modulemd_module_stream_build_depends_on_stream (stream, "base", "f30"));
  g_clear_object (&stream);
}

static void
module_stream_v2_test_validate_buildafter (ModuleStreamFixture *fixture,
                                           gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr (GError) error = NULL;

  /* Test a valid module stream with buildafter set */
  path = g_strdup_printf ("%s/buildafter/good_buildafter.yaml",
                          g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_nonnull (stream);
  g_assert_null (error);
  g_clear_pointer (&path, g_free);
  g_clear_object (&stream);

  /* Should fail validation if both buildorder and buildafter are set for the
   * same component.
   */
  path = g_strdup_printf ("%s/buildafter/both_same_component.yaml",
                          g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_error (error, MODULEMD_ERROR, MODULEMD_ERROR_VALIDATE);
  g_assert_null (stream);
  g_clear_error (&error);
  g_clear_pointer (&path, g_free);

  /* Should fail validation if both buildorder and buildafter are set in
   * different components of the same stream.
   */
  path = g_strdup_printf ("%s/buildafter/mixed_buildorder.yaml",
                          g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_error (error, MODULEMD_ERROR, MODULEMD_ERROR_VALIDATE);
  g_assert_null (stream);
  g_clear_error (&error);
  g_clear_pointer (&path, g_free);

  /* Should fail if a key specified in a buildafter set does not exist for this
   * module stream.
   */
  path = g_strdup_printf ("%s/buildafter/invalid_key.yaml",
                          g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_error (error, MODULEMD_ERROR, MODULEMD_ERROR_VALIDATE);
  g_assert_null (stream);
  g_clear_error (&error);
  g_clear_pointer (&path, g_free);
}


static void
module_stream_v2_test_validate_buildarches (ModuleStreamFixture *fixture,
                                            gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr (GError) error = NULL;

  /* Test a valid module stream with no buildopts or component
   * rpm arches set.
   */
  path = g_strdup_printf ("%s/buildarches/good_no_arches.yaml",
                          g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_nonnull (stream);
  g_assert_null (error);
  g_clear_pointer (&path, g_free);
  g_clear_object (&stream);

  /* Test a valid module stream with buildopts arches but no component rpm
   * arches set.
   */
  path = g_strdup_printf ("%s/buildarches/only_module_arches.yaml",
                          g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_nonnull (stream);
  g_assert_null (error);
  g_clear_pointer (&path, g_free);
  g_clear_object (&stream);

  /* Test a valid module stream with component rpm arches but no buildopts
   * arches set.
   */
  path = g_strdup_printf ("%s/buildarches/only_rpm_arches.yaml",
                          g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_nonnull (stream);
  g_assert_null (error);
  g_clear_pointer (&path, g_free);
  g_clear_object (&stream);

  /* Test a valid module stream with buildopts arches set and a component rpm
   * specified containing a subset of archs specified at the module level.
   */
  path = g_strdup_printf ("%s/buildarches/good_combo_arches.yaml",
                          g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_nonnull (stream);
  g_assert_null (error);
  g_clear_pointer (&path, g_free);
  g_clear_object (&stream);

  /* Should fail validation if buildopts arches is set and a component rpm
   * specified an arch not specified at the module level.
   */
  path = g_strdup_printf ("%s/buildarches/bad_combo_arches.yaml",
                          g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_error (error, MODULEMD_ERROR, MODULEMD_ERROR_VALIDATE);
  g_assert_null (stream);
  g_clear_error (&error);
  g_clear_pointer (&path, g_free);
}


static void
module_stream_v2_test_rpm_map (ModuleStreamFixture *fixture,
                               gconstpointer user_data)
{
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  g_autoptr (ModulemdRpmMapEntry) entry = NULL;
  ModulemdRpmMapEntry *retrieved_entry = NULL;

  stream = modulemd_module_stream_v2_new ("foo", "bar");
  g_assert_nonnull (stream);

  entry = modulemd_rpm_map_entry_new (
    "bar", 0, "1.23", "1.module_deadbeef", "x86_64");
  g_assert_nonnull (entry);

  modulemd_module_stream_v2_set_rpm_artifact_map_entry (
    stream, entry, "sha256", "baddad");

  retrieved_entry = modulemd_module_stream_v2_get_rpm_artifact_map_entry (
    stream, "sha256", "baddad");
  g_assert_nonnull (retrieved_entry);

  g_assert_true (modulemd_rpm_map_entry_equals (entry, retrieved_entry));
}

static void
module_stream_v2_test_unicode_desc (void)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr (GError) error = NULL;

  /* Test a module stream with unicode in description */
  path =
    g_strdup_printf ("%s/stream_unicode.yaml", g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);

  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_nonnull (stream);
  g_assert_no_error (error);
}


static void
module_stream_v2_test_xmd_issue_274 (void)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr (GError) error = NULL;
  GVariant *xmd1 = NULL;
  GVariant *xmd2 = NULL;

  path =
    g_strdup_printf ("%s/stream_unicode.yaml", g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);

  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_nonnull (stream);
  g_assert_no_error (error);
  g_assert_cmpint (modulemd_module_stream_get_mdversion (stream),
                   ==,
                   MD_MODULESTREAM_VERSION_ONE);

  xmd1 =
    modulemd_module_stream_v1_get_xmd (MODULEMD_MODULE_STREAM_V1 (stream));
  xmd2 =
    modulemd_module_stream_v1_get_xmd (MODULEMD_MODULE_STREAM_V1 (stream));

  g_assert_true (xmd1 == xmd2);
}


static void
module_stream_v2_test_xmd_issue_290 (void)
{
  g_auto (GVariantBuilder) builder;
  g_autoptr (GVariant) xmd = NULL;
  GVariant *xmd_array = NULL;
  g_autoptr (GVariantDict) xmd_dict = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (ModulemdModuleIndex) index = modulemd_module_index_new ();
  g_autofree gchar *yaml_str = NULL;

  g_autoptr (ModulemdModuleStreamV2) stream =
    modulemd_module_stream_v2_new ("foo", "bar");

  modulemd_module_stream_v2_set_summary (stream, "summary");
  modulemd_module_stream_v2_set_description (stream, "desc");
  modulemd_module_stream_v2_add_module_license (stream, "MIT");


  g_variant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);

  g_variant_builder_add_value (&builder, g_variant_new_string ("foo"));
  g_variant_builder_add_value (&builder, g_variant_new_string ("bar"));

  xmd_array = g_variant_builder_end (&builder);

  xmd_dict = g_variant_dict_new (NULL);
  g_variant_dict_insert_value (
    xmd_dict, "something", g_steal_pointer (&xmd_array));
  xmd = g_variant_ref_sink (g_variant_dict_end (xmd_dict));


  modulemd_module_stream_v2_set_xmd (stream, xmd);

  g_assert_true (modulemd_module_index_add_module_stream (
    index, MODULEMD_MODULE_STREAM (stream), &error));
  g_assert_no_error (error);

  yaml_str = modulemd_module_index_dump_to_string (index, &error);

  // clang-format off
  g_assert_cmpstr (yaml_str, ==,
"---\n"
"document: modulemd\n"
"version: 2\n"
"data:\n"
"  name: foo\n"
"  stream: bar\n"
"  summary: summary\n"
"  description: >-\n"
"    desc\n"
"  license:\n"
"    module:\n"
"    - MIT\n"
"  xmd:\n"
"    something:\n"
"    - foo\n"
"    - bar\n"
"...\n");
  // clang-format on

  g_assert_no_error (error);
}


static void
module_stream_v2_test_xmd_issue_290_with_example (void)
{
  g_autoptr (ModulemdModuleStream) stream = NULL;
  g_autofree gchar *path = NULL;
  g_autoptr (GError) error = NULL;
  g_autoptr (ModulemdModuleIndex) index = modulemd_module_index_new ();
  g_autofree gchar *output_yaml = NULL;
  g_autoptr (GVariant) xmd = NULL;

  path = g_strdup_printf ("%s/290.yaml", g_getenv ("TEST_DATA_PATH"));
  g_assert_nonnull (path);
  stream = modulemd_module_stream_read_file (path, TRUE, NULL, NULL, &error);
  g_assert_nonnull (stream);
  g_assert_no_error (error);

  xmd = modulemd_variant_deep_copy (
    modulemd_module_stream_v1_get_xmd (MODULEMD_MODULE_STREAM_V1 (stream)));
  modulemd_module_stream_v1_set_xmd (MODULEMD_MODULE_STREAM_V1 (stream), xmd);

  g_assert_true (
    modulemd_module_index_add_module_stream (index, stream, &error));
  g_assert_no_error (error);

  output_yaml = modulemd_module_index_dump_to_string (index, &error);
  g_assert_nonnull (output_yaml);
  g_assert_no_error (error);
}

static void
module_stream_v1_test_community (void)
{
  g_autoptr (ModulemdModuleStreamV1) stream = NULL;
  const gchar *community = NULL;
  g_autofree gchar *community_prop = NULL;

  stream = modulemd_module_stream_v1_new (NULL, NULL);

  // Check the defaults
  community = modulemd_module_stream_v1_get_community (stream);
  g_object_get (stream, MMD_TEST_COM_PROP, &community_prop, NULL);
  g_assert_null (community);
  g_assert_null (community_prop);

  g_clear_pointer (&community_prop, g_free);

  // Test property setting
  g_object_set (stream, MMD_TEST_COM_PROP, MMD_TEST_DOC_TEXT, NULL);

  community = modulemd_module_stream_v1_get_community (stream);
  g_object_get (stream, MMD_TEST_COM_PROP, &community_prop, NULL);
  g_assert_cmpstr (community_prop, ==, MMD_TEST_DOC_TEXT);
  g_assert_cmpstr (community, ==, MMD_TEST_DOC_TEXT);

  g_clear_pointer (&community_prop, g_free);

  // Test set_community()
  modulemd_module_stream_v1_set_community (stream, MMD_TEST_DOC_TEXT2);

  community = modulemd_module_stream_v1_get_community (stream);
  g_object_get (stream, MMD_TEST_COM_PROP, &community_prop, NULL);
  g_assert_cmpstr (community_prop, ==, MMD_TEST_DOC_TEXT2);
  g_assert_cmpstr (community, ==, MMD_TEST_DOC_TEXT2);

  g_clear_pointer (&community_prop, g_free);

  // Test setting to NULL
  g_object_set (stream, MMD_TEST_COM_PROP, NULL, NULL);

  community = modulemd_module_stream_v1_get_community (stream);
  g_object_get (stream, MMD_TEST_COM_PROP, &community_prop, NULL);
  g_assert_null (community);
  g_assert_null (community_prop);

  g_clear_pointer (&community_prop, g_free);
  g_clear_object (&stream);
}

static void
module_stream_v2_test_community (void)
{
  g_autoptr (ModulemdModuleStreamV2) stream = NULL;
  const gchar *community = NULL;
  g_autofree gchar *community_prop = NULL;

  stream = modulemd_module_stream_v2_new (NULL, NULL);

  // Check the defaults
  community = modulemd_module_stream_v2_get_community (stream);
  g_object_get (stream, MMD_TEST_COM_PROP, &community_prop, NULL);
  g_assert_null (community);
  g_assert_null (community_prop);

  g_clear_pointer (&community_prop, g_free);

  // Test property setting
  g_object_set (stream, MMD_TEST_COM_PROP, MMD_TEST_DOC_TEXT, NULL);

  community = modulemd_module_stream_v2_get_community (stream);
  g_object_get (stream, MMD_TEST_COM_PROP, &community_prop, NULL);
  g_assert_cmpstr (community_prop, ==, MMD_TEST_DOC_TEXT);
  g_assert_cmpstr (community, ==, MMD_TEST_DOC_TEXT);

  g_clear_pointer (&community_prop, g_free);

  // Test set_community()
  modulemd_module_stream_v2_set_community (stream, MMD_TEST_DOC_TEXT2);

  community = modulemd_module_stream_v2_get_community (stream);
  g_object_get (stream, MMD_TEST_COM_PROP, &community_prop, NULL);
  g_assert_cmpstr (community_prop, ==, MMD_TEST_DOC_TEXT2);
  g_assert_cmpstr (community, ==, MMD_TEST_DOC_TEXT2);

  g_clear_pointer (&community_prop, g_free);

  // Test setting to NULL
  g_object_set (stream, MMD_TEST_COM_PROP, NULL, NULL);

  community = modulemd_module_stream_v2_get_community (stream);
  g_object_get (stream, MMD_TEST_COM_PROP, &community_prop, NULL);
  g_assert_null (community);
  g_assert_null (community_prop);

  g_clear_pointer (&community_prop, g_free);
  g_clear_object (&stream);
}

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");
  g_test_init (&argc, &argv, NULL);
  g_test_bug_base ("https://bugzilla.redhat.com/show_bug.cgi?id=");

  // Define the tests.o

  g_test_add ("/modulemd/v2/modulestream/construct",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_test_construct,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/arch",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_test_arch,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/documentation",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_documentation,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/documentation",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_documentation,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/licenses",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_licenses,
              NULL);


  g_test_add ("/modulemd/v2/modulestream/v2/licenses",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_licenses,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/tracker",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_tracker,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/tracker",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_tracker,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/profiles",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_profiles,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/profiles",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_profiles,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/rpm_api",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_rpm_api,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/rpm_api",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_rpm_api,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/rpm_filters",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_rpm_filters,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/rpm_filters",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_rpm_filters,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/upgrade",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_test_upgrade,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/rpm_artifacts",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_rpm_artifacts,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/rpm_artifacts",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_rpm_artifacts,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/components",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_components,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/components",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_components,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/copy",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_test_copy,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/equals",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_test_equals,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/nsvc",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_test_nsvc,
              NULL);


  g_test_add ("/modulemd/v2/modulestream/nsvca",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_test_nsvca,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/equals",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_equals,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/equals",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_equals,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/dependencies",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_dependencies,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/dependencies",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_dependencies,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/parse_dump",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_parse_dump,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/parse_dump",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_parse_dump,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v1/depends_on_stream",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v1_test_depends_on_stream,
              NULL);
  g_test_add ("/modulemd/v2/modulestream/v2/depends_on_stream",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_depends_on_stream,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/validate/buildafter",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_validate_buildafter,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/validate/buildarches",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_validate_buildarches,
              NULL);

  g_test_add ("/modulemd/v2/modulestream/v2/rpm_map",
              ModuleStreamFixture,
              NULL,
              NULL,
              module_stream_v2_test_rpm_map,
              NULL);

  g_test_add_func ("/modulemd/v2/modulestream/v1/community",
                   module_stream_v1_test_community);

  g_test_add_func ("/modulemd/v2/modulestream/v2/community",
                   module_stream_v2_test_community);

  g_test_add_func ("/modulemd/v2/modulestream/v2/unicode/description",
                   module_stream_v2_test_unicode_desc);

  g_test_add_func ("/modulemd/v2/modulestream/v2/xmd/issue274",
                   module_stream_v2_test_xmd_issue_274);

  g_test_add_func ("/modulemd/v2/modulestream/v2/xmd/issue290",
                   module_stream_v2_test_xmd_issue_290);

  g_test_add_func ("/modulemd/v2/modulestream/v2/xmd/issue290plus",
                   module_stream_v2_test_xmd_issue_290_with_example);

  return g_test_run ();
}
