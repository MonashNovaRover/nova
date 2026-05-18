# XML Schemas for KCD Files

You want to know what can/can't be added to different parts of a `.kcd` file? What it's structure needs to be? Well you can use an "XML Schema" for that; it defines what elements can go where and what attributes each element can have.

For `.kcd` files, you can find it's schema [here](https://github.com/dschanoeh/Kayak/blob/master/Kayak-kcd/src/main/resources/com/github/kayak/canio/kcd/loader/Definition.xsd) (we technically can't add this to our repo as it's license is likely incompatible). If you use CLion, it should be able to automatically check if your `.kcd` files comply with this schema; and give you warnings/errors if it doesn't. Note that we use xacro to generate our `.kcd` files, so the xacro files themselves don't comply with this schema; but the final generated result should.

Additionally, an example `.kcd` file is provided [here](https://github.com/dschanoeh/Kayak/blob/master/Kayak-kcd/src/test/resources/can_definition_sample.kcd) (or you can look at one of the automatically generated ones in one of your builds).