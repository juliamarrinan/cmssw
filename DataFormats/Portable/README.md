## Define portable data formats that wrap a simple C-style `struct` and can be persisted to ROOT files

### `PortableHostObject<T>`

`PortableHostObject<T>` is a class template that wraps a `struct` type `T` and an alpaka host buffer, that owns the
memory where the `struct` is allocated. The content of the `struct` is persistent, while the buffer itself is transient.
Specialisations of this template can be persisted, but requre special ROOT read rules to read the data into an alpaka
memory buffer.

The traditional way to declare these rules would be to add them to the `classes_def.xml` file. This method is described
here for reference, but is deprecated. See below for a simpler method.
For example, to declare the read rules for `portabletest::TestHostObject` based on the `portabletest::TestStruct` `struct`,
one would add to the same `classes_def.xml` file where `portabletest::TestHostObject` is declared:
```xml
<read
  sourceClass="portabletest::TestHostObject"
  targetClass="portabletest::TestHostObject"
  version="[1-]"
  source="portabletest::TestStruct* product_;"
  target="buffer_,product_"
  embed="false">
<![CDATA[
  portabletest::TestHostObject::ROOTReadStreamer(newObj, *onfile.product_);
]]>
</read>
```

The recommended way to declare these rules is to create a `classes.cc` file along with `classes.h`, and use the
`SET_PORTABLEHOSTOBJECT_READ_RULES` macro. For example, to declare the rules for `portabletest::TestHostObject` one
would create the file `classes.cc` with the content:
```c++
#include "DataFormats/Portable/interface/PortableHostObjectReadRules.h"
#include "DataFormats/PortableTestObjects/interface/TestHostObject.h"

SET_PORTABLEHOSTOBJECT_READ_RULES(portabletest::TestHostObject);
```

`PortableHostObject<T>` objects can also be read back in "bare ROOT" mode, without any dictionaries.
They have no implicit or explicit references to alpaka (neither as part of the class signature nor as part of its name).
This could make it possible to read them back with different portability solutions in the future.

### `PortableDeviceObject<T, TDev>`

`PortableDeviceObject<T, TDev>` is a class template that wraps a `struct` type `T` and an alpaka device buffer, that
owns the memory where the `struct` is allocated.
To avoid confusion and ODR-violations, the `PortableDeviceObject<T, TDev>` template cannot be used with the `Host`
device type.
Specialisations of this template are transient and cannot be persisted.

### `ALPAKA_ACCELERATOR_NAMESPACE::PortableObject<T>`

`ALPAKA_ACCELERATOR_NAMESPACE::PortableObject<T>` is a template alias that resolves to either
`PortableHostObject<T>` or `PortableDeviceObject<T, ALPAKA_ACCELERATOR_NAMESPACE::Device>`, depending on the
backend.

### `PortableObject<T, TDev>`

`PortableObject<T, TDev>` is an alias template that resolves to `ALPAKA_ACCELERATOR_NAMESPACE::PortableObject<T>`
for the matching device.


## Define portable data formats that wrap SoA data structures and can be persisted to ROOT files

### `PortableHostCollection<T>`

`PortableHostCollection<T>` is a class template that wraps a SoA type `T` and an alpaka host buffer, which owns the
memory where the SoA is allocated. The content of the SoA is persistent, while the buffer itself is transient.
Specialisations of this template can be persisted, but requre special ROOT read rules to read the data into a single
memory buffer.

The original way to declare these rules, now deprecated, is to add them to the `classes_def.xml` file. For example,
to declare the read rules for `portabletest::TestHostCollection` based on the `portabletest::TestSoA` SoA, one
would add to the same `classes_def.xml` file where `portabletest::TestHostCollection` is declared:
```xml
<read
  sourceClass="portabletest::TestHostCollection"
  targetClass="portabletest::TestHostCollection"
  version="[1-]"
  source="portabletest::TestSoA layout_;"
  target="buffer_,layout_,view_"
  embed="false">
<![CDATA[
  portabletest::TestHostCollection::ROOTReadStreamer(newObj, onfile.layout_);
]]>
```

The new, recommended way to declare these rules is to create a `classes.cc` file along with `classes.h`, and use the
`SET_PORTABLEHOSTCOLLECTION_READ_RULES` macro. For example, to declare the rules for `portabletest::TestHostCollection`
one would create the file `classes.cc` with the content:
```c++
#include "DataFormats/Portable/interface/PortableHostCollectionReadRules.h"
#include "DataFormats/PortableTestObjects/interface/TestHostCollection.h"

SET_PORTABLEHOSTCOLLECTION_READ_RULES(portabletest::TestHostCollection);
```

`PortableHostCollection<T>` collections can also be read back in "bare ROOT" mode, without any dictionaries.
They have no implicit or explicit references to alpaka (neither as part of the class signature nor as part of its name).
This could make it possible to read them back with different portability solutions in the future.

The member function `void deepCopy(ConstView const& view)` copies the content of all scalars and columns from `view`
(pointing to data in host memory and potentially to multiple buffers) into the `PortableHostCollection` contiguous buffer.
This method should be used for host to host data transfer. See the [`View` section](../../DataFormats/SoATemplate/README.md#view)
of [`DataFormats/SoATemplate/README.md`](../../DataFormats/SoATemplate/README.md) for more details.

The member function `void deepCopy(ConstView const& src, TQueue& queue)` copies the content of all scalars and columns 
from a `View` or `ConstView` (pointing to data in host/device memory and potentially to multiple buffers), passing through
a `ConstDescriptor` object, into the `PortableHostCollection` contigous buffer. This method should be used for device to
host data transfer.

### `PortableDeviceCollection<T, TDev>`

`PortableDeviceCollection<T, TDev>` is a class template that wraps a SoA type `T` and an alpaka device buffer, which
owns the memory where the SoA is allocated.
To avoid confusion and ODR-violations, the `PortableDeviceCollection<T, TDev>` template cannot be used with the `Host`
device type.
Specialisations of this template are transient and cannot be persisted.

The member function `void deepCopy(ConstView const& src, TQueue& queue)` copies the content of all scalars and columns 
from a `View` or `ConstView` (pointing to data in host/device memory and potentially to multiple buffers), passing through
a `ConstDescriptor` object into the `PortableDeviceCollection` contigous buffer. This method should be used for 
host to device or device to device data transfer.

### `ALPAKA_ACCELERATOR_NAMESPACE::PortableCollection<T>`

`ALPAKA_ACCELERATOR_NAMESPACE::PortableCollection<T>` is a template alias that resolves to either
`PortableHostCollection<T>` or `PortableDeviceCollection<T, ALPAKA_ACCELERATOR_NAMESPACE::Device>`, depending on the
backend.

### `PortableCollection<T, TDev>`

`PortableCollection<T, TDev>` is an alias template that resolves to `ALPAKA_ACCELERATOR_NAMESPACE::PortableCollection<T>`
for the matching device.

## Notes

Modules that are supposed to work with only host types (_e.g._ dealing with de/serialisation, data transfers, _etc._)
should explicitly use the `PortableHostObject<T>` and `PortableHostCollection<T>` types.

Modules that implement portable interfaces (_e.g._ producers) should use the generic types based on
`ALPAKA_ACCELERATOR_NAMESPACE::PortableObject<T>` or `PortableObject<T, TDev>`, and
`ALPAKA_ACCELERATOR_NAMESPACE::PortableCollection<T>` or `PortableCollection<T, TDev>`.

## Multi layout collections

Some use cases require multiple sets of columns of different sizes. This is can be achieved in a single
`PortableCollection` using `PortableCollection2<T1, T2>`, `PortableCollection3<T1, T2, T3>` and so on up to
`PortableCollection5<...>`. The numbered, fixed size wrappers are needed in order to be added to the ROOT dictionary.
Behind the scenes recursive `PortableHostMultiCollection<T0, ...>` and
`ALPAKA_ACCELERATOR_NAMESPACE::PortableDeviceMultiCollection<TDev, T0, ...>` (note the reversed parameter order) provide
the actual class definitions.

## ROOT dictionary declaration helper scripts

In order to be serialized by ROOT, the products need to be added to its dictionary. This happens during `scram build`
as instructed in `<module>/src/classes_dev.xml` and `<module>/src/alpaka/classes_cuda_def.xml` and
`<module>/src/alpaka/classes_rocm_def.xml`. Two scripts generate the code to be added to the xml files.
Both scripts expect the collections to be aliased as in:
```
using TestDeviceMultiCollection3 = PortableCollection3<TestSoA, TestSoA2, TestSoA3>;
```

For the host xml, SoA layouts have to be listed and duplicates should be removed manually is multiple
collections share a same layout. The scripts are called as follows:
```
./DataFormats/Portable/scripts/portableHostCollectionHints portabletest::TestHostMultiCollection3  \
            portabletest::TestSoALayout portabletest::TestSoALayout2 portabletest::TestSoALayout3

./DataFormats/Portable/scripts/portableDeviceCollectionHints portabletest::TestHostMultiCollection3
```
The layouts should not be added as parameters for the device collection. Those script can be use equally with the
single layout collections or multi layout collections.
