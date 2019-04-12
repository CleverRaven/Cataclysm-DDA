### When you run the application:

The first function called is **initBasePath( path )**, the second function
in call order is **initUserDirectory( path )**, the third function in call
order is **setStandardFilenames()**.

```cpp

int main (int argc, char *argv[])
{
    Path::initBasePath( path );
    Path::initUserDirectory( path );
    Path::setStandardFilenames();
}

```

---

### Call from `Path::initBasePath` or `Path::initUserDirectory`

After calling **initBasePath(newPath)** and/or **initUserDirectory
(newPath)**, either jointly or separately it is necessary to call
**SetStandardFilenames()** to apply the changes.

```cpp

    Path::initBasePath( newPath );
    Path::setStandardFilenames();

```

```cpp

    Path::initUserDirectory( newPath );
    Path::setStandardFilenames();

```

---

### Updating the path name "datadir" with the `Path::updatePathname( key, newPath )` function

Update the path name _"datadir"_ with the function **updatePathname
("datadir", newPath)** necessarily involves calling the
function **updateDataDirectory()**.

```cpp

    Path::updatePathname("datadir", newPath);
    Path::updateDataDirectory();

```

----

### Path name update "config_dir" with `Path::updatePathname( key, newPath )` function

Update the path name _"config_dir"_ with the function **updatePathname
("config_dir", newPath)** necessarily involves calling the function
**updateConfigurationDiretory()**.

```cpp

    Path::updatePathname("config_dir", newPath);
    Path::updateConfigurationDirectory();

```

---
