import os

# List of callables that know how to import templates from various sources.
TEMPLATE_LOADERS = (
    'django.template.loaders.filesystem.Loader',
)
TEMPLATE_DIRS = (
    # Note: Django source files are here:
    # /usr/local/lib/python2.7/dist-packages/django
    # Put strings here, like "/home/html/django_templates" or
    # "C:/www/django/templates".
    # Always use forward slashes, even on Windows.
    # Don't forget to use absolute paths, not relative paths.
    os.path.realpath('.'),
)

SECRET_KEY = 'reallysecret'
