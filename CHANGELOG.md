# CHANGELOG

## [2.2.dev1](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.2.dev1) - 2024-09-21

- [42d2626](http://github.com/CraterCrash/godot-orchestrator/commit/42d26265055423f3bedf6d2c8736936db17973b9) - GH-724 Fix copy-n-paste between two orchestrations
- [4e2824a](http://github.com/CraterCrash/godot-orchestrator/commit/4e2824a8173a5e06cfcd0f67d575837ee402ead8) - GH-807 Support named lookup of signals, used by `await` keyword
- [704cfba](http://github.com/CraterCrash/godot-orchestrator/commit/704cfbaef8961b06a1a183a3f8a01c02f16f4371) - GH-714 Support `ui_cut` action in `GraphEdit` controls
- [2901320](http://github.com/CraterCrash/godot-orchestrator/commit/29013204ce935ecc65741ba27d6e27399647b949) - GH-618 Reset pin default on connection
- [bdd11a0](http://github.com/CraterCrash/godot-orchestrator/commit/bdd11a0a9b823873b871d1d1e36fa6afecb7fbf9) - GH-670 Fix reset pin to default value
- [fef3e93](http://github.com/CraterCrash/godot-orchestrator/commit/fef3e93787de912233d5a0fe53e658002c6db660) - GH-803 Component panels react to context-menu shortcuts
- [32a2b02](http://github.com/CraterCrash/godot-orchestrator/commit/32a2b02bdab4b5fca51d1229389a5f4c9763b003) - GH-761 Use new ClassDB methods to fetch property getter/setter
- [57c0207](http://github.com/CraterCrash/godot-orchestrator/commit/57c0207a3b166f67bc6b3fa49c5841a5e4a12dab) - GH-800 Temporarily disable some property get/set validation rules
- [5da872b](http://github.com/CraterCrash/godot-orchestrator/commit/5da872b7f6134661a013eaaafb64208498d7da55) - GH-798 Remove unnecessary instantiate scene cache
- [dc0b0f9](http://github.com/CraterCrash/godot-orchestrator/commit/dc0b0f9a5c3e4721b69060a305fa335cef6039a2) - GH-796 Guard against `get_current_scene` returning null
- [d5558ee](http://github.com/CraterCrash/godot-orchestrator/commit/d5558eed96faef74eb7e11d1afd3b8fba27fd3bd) - GH-794 Fix default value evaluation in ScriptPlaceHolderInstance
- [30eea75](http://github.com/CraterCrash/godot-orchestrator/commit/30eea75518767f382824df6f77a113f9e7c32a99) - Build system cleanup and refactor
- [5a0d577](http://github.com/CraterCrash/godot-orchestrator/commit/5a0d5773cad9f2df761e21c2791dfde3efad5660) - GH-467 Only save node size when manually resized (Godot 4.3+)
- [a994dff](http://github.com/CraterCrash/godot-orchestrator/commit/a994dff4d479f36f817f6033a7398fc817866327) - GH-786 Add tooltip for `Orchestration Build` button
- [965fec3](http://github.com/CraterCrash/godot-orchestrator/commit/965fec33ef33a57127eeb65ab019a994d158c2c3) - GH-778 Fix crash on Windows 10 - Revert GH-653
- [7bbc5a1](http://github.com/CraterCrash/godot-orchestrator/commit/7bbc5a142ed1c29b6198e10b70a525a840a23f1f) - Prefer `&&` rather than `and` operator
- [3307fd1](http://github.com/CraterCrash/godot-orchestrator/commit/3307fd16a3ac9d6fd1f920ebe19e311ab634b06f) - GH-774 Relax return node validation for sequence nodes
- [301fd21](http://github.com/CraterCrash/godot-orchestrator/commit/301fd21f37c5bf3fed16bf12d8ef157c0659c162) - GH-771 Make exported variables searchable in `All Actions`
- [e23dbfe](http://github.com/CraterCrash/godot-orchestrator/commit/e23dbfeaafef74055bf10d2970a0a0bf2bdae35f) - [ci] Cleanup CI build configuration
- [3312036](http://github.com/CraterCrash/godot-orchestrator/commit/331203666e40637ce2944401e4136bdaea51f137) - GH-765 Disable debug symbols on Linux/Android builds
- [542ce01](http://github.com/CraterCrash/godot-orchestrator/commit/542ce01fabf5e320737269633ce7e483236c86bb) - GH-427 Support custom icons for comment nodes
- [3bccbb6](http://github.com/CraterCrash/godot-orchestrator/commit/3bccbb67379900a164144f4f48357b5535bbd2de) - GH-756 Fix validated variable getter state consistency
- [6dff26d](http://github.com/CraterCrash/godot-orchestrator/commit/6dff26d764c2a1cea7edacfa9acd8ca9a79dccd5) - GH-712 Clamp `All Actions` dialog within screen rect.
- [4301d24](http://github.com/CraterCrash/godot-orchestrator/commit/4301d24acbd4ce0082addb8e6ff8fad3c0b481a0) - GH-752 Restrict arrow-key movement to only selected elements
- [1953283](http://github.com/CraterCrash/godot-orchestrator/commit/1953283d359809cedf0486373a76580339f53841) - GH-736 Fix knot movement/alignment
- [0140edd](http://github.com/CraterCrash/godot-orchestrator/commit/0140eddf2b4a54381c82f77e55ba3178e37ad2b4) - GH-737 Allow knot node operations within comment nodes
- [106e0e5](http://github.com/CraterCrash/godot-orchestrator/commit/106e0e5c3afdf669e0a92a69172714097500271e) - GH-738 Fix `Select Group` to include Knots and Nodes
- [0333834](http://github.com/CraterCrash/godot-orchestrator/commit/03338348e11a0e820e93a6e75341f2325010417b) - GH-655 Use `EditorInterface::get_singleton` where possible
- [ad909cf](http://github.com/CraterCrash/godot-orchestrator/commit/ad909cf29f978e8fd5baeca69c8fdb5a0d87d74c) - GH-743 Fix update picker is_after logic

## [2.1.stable](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.1.stable) - 2024-08-15

- [0413239](http://github.com/CraterCrash/godot-orchestrator/commit/041323953b3accec5ab93e954faa6c2a87678943) - GH-741 Bump godot-engine/godot-cpp to 4.3.stable
- [20b5362](http://github.com/CraterCrash/godot-orchestrator/commit/20b5362e615044adb3e7b24e52a8fcc58015867a) - GH-716 Include knots in movement with arrow keys
- [94b0204](http://github.com/CraterCrash/godot-orchestrator/commit/94b02042f96641d3eac2cab55d613afbf377a6e3) - Fix broken help
- [45496a9](http://github.com/CraterCrash/godot-orchestrator/commit/45496a9880762aab3ad5d41cf2a4e6fb07a0212e) - GH-727 Add constant variable support
- [23d6427](http://github.com/CraterCrash/godot-orchestrator/commit/23d64278ed633189cbdc43388bedb770cc29064d) - GH-722 Correctly copy node state using `ui_copy` action
- [8f882c9](http://github.com/CraterCrash/godot-orchestrator/commit/8f882c9c058ab15a4d87eb5ea3d3990df0dca94e) - GH-715 Alphabetically sort singleton list in inspector
- [c163185](http://github.com/CraterCrash/godot-orchestrator/commit/c163185edefccaea83e56f66c6d855fedb0ae13b) - GH-729 Graph grid settings & cache left/right panel layouts
- [b8b306a](http://github.com/CraterCrash/godot-orchestrator/commit/b8b306a37cee4909373f84119d5acbaa757c23e9) - GH-718 Add variable descriptions to get/set node tooltips
- [a01c35d](http://github.com/CraterCrash/godot-orchestrator/commit/a01c35d502b7646350d7d9b197dcbeeff2dfbc15) - GH-716 Allow moving nodes using arrow keys
- [34f0374](http://github.com/CraterCrash/godot-orchestrator/commit/34f037445b17e284404638afd6a940bf876dcd39) - Set min height on Orchestration build output panel
- [112cc20](http://github.com/CraterCrash/godot-orchestrator/commit/112cc201d6dd8d996f3146c86279d8053528196c) - GH-705 Correctly size controls based on editor scale
- [f727883](http://github.com/CraterCrash/godot-orchestrator/commit/f727883e9b4e7d0cf5830c32232bf06c809d0b3f) - GH-707 Add GraphNode alignment options
- [c280586](http://github.com/CraterCrash/godot-orchestrator/commit/c280586ea802e6bf2c71a230f3d36a63e7db0d04) - GH-680 Fix crash on load
- [57ec042](http://github.com/CraterCrash/godot-orchestrator/commit/57ec042d66b49f6886408cf675ae034568d0d19c) - GH-680 Support several serialization methods: `get_dependencies`, `rename_dependencies`, and `get_resource_script_class`
- [bb10544](http://github.com/CraterCrash/godot-orchestrator/commit/bb10544bdbd3d8a79b7bb36e98c7e95521a33fa3) - GH-680 Fix loading external resources
- [beeeb84](http://github.com/CraterCrash/godot-orchestrator/commit/beeeb847def000c0a30e1b1aa74ec70d3a02cf0f) - GH-703 Update FileSystemDock on file save as
- [20ba58a](http://github.com/CraterCrash/godot-orchestrator/commit/20ba58a9194853486d760f328382e3d1dad2a0b8) - GH-693 Disable conversion to binary format on export
- [11ddf96](http://github.com/CraterCrash/godot-orchestrator/commit/11ddf96977b40f67f6de966d289614adabcaf1d2) - GH-693 Emit resolved target object for method chain
- [bb5cb4d](http://github.com/CraterCrash/godot-orchestrator/commit/bb5cb4d436522ddeadb8cbb36f5cfb19de69bbc4) - GH-701 Fix type cast honoring global script classes
- [e539db6](http://github.com/CraterCrash/godot-orchestrator/commit/e539db620783516c23dbf7695379ff9b33402b5d) - Update copyright headers
- [32c9ea3](http://github.com/CraterCrash/godot-orchestrator/commit/32c9ea34432dba333de3f4df74c5a8111021951a) - GH-697 Synchronize graph on theme changes
- [cccc3a5](http://github.com/CraterCrash/godot-orchestrator/commit/cccc3a542587690e4998ee7c43e7bc93a60082cd) - GH-696 Avoid error when disabling recent files option
- [7190c7c](http://github.com/CraterCrash/godot-orchestrator/commit/7190c7c388610d610186b965701df56f5aa96131) - GH-684 Fix call mode semantics
- [98eedaa](http://github.com/CraterCrash/godot-orchestrator/commit/98eedaa05c40f7045e00d58b37925873d82d2167) - GH-640 Fix Godot 4.2 compatibility
- [adf4c7e](http://github.com/CraterCrash/godot-orchestrator/commit/adf4c7eb1a663497abf8fa8639e24fee7d77c82a) - GH-640 Add dragging function creates callable
- [90120ad](http://github.com/CraterCrash/godot-orchestrator/commit/90120ad22d48d574711526423cacf1b3392a712a) - GH-689 Introduce `PackedVector4Array` serialization handlers
- [cecce3c](http://github.com/CraterCrash/godot-orchestrator/commit/cecce3c66d197f2e1e22d1ee181755b1ef7fb726) - GH-691 Use function argument names rather than class names
- [624856e](http://github.com/CraterCrash/godot-orchestrator/commit/624856e9144c03f6b029f89b52c9b9befaeba2b6) - GH-682 Fix type resolution
- [af6f01a](http://github.com/CraterCrash/godot-orchestrator/commit/af6f01a80a3ab883f5b07ab727e9b4b1025b4358) - GH-682 Fix exporting `Node` based variable types
- [0b4de3a](http://github.com/CraterCrash/godot-orchestrator/commit/0b4de3ab75e62ddd2970e12be8a0a0a4bec322a0) - GH-687 Fix usage of script vs p_script
- [44e310d](http://github.com/CraterCrash/godot-orchestrator/commit/44e310d6054f913c7b3a051f4e83129c586d7895) - GH-687 Check script type on debugger signals

## [2.1.rc4](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.1.rc4) - 2024-08-08

- [cfa9b38](http://github.com/CraterCrash/godot-orchestrator/commit/cfa9b3885b08e9cda6e6803f53999e9a3ecd143e) - GH-657 Bump godot-engine/godot-cpp to 4.3.rc2
- [24eb6e7](http://github.com/CraterCrash/godot-orchestrator/commit/24eb6e7491c05a222c204752cd8e0b0c9c66563b) - GH-648 Permit any output to connect to boolean input
- [2a7c311](http://github.com/CraterCrash/godot-orchestrator/commit/2a7c31129718b8c02665f0eafdaf0c3590f0452f) - GH-636 Don't wrap text in `PrintStringUI`
- [f2289db](http://github.com/CraterCrash/godot-orchestrator/commit/f2289db71f53be4270c990fd047f3c8c137eb743) - GH-674 Fix Await signal to work with parameterized signals
- [c1d1a24](http://github.com/CraterCrash/godot-orchestrator/commit/c1d1a245f8cb3750035e99f3f9386444c117a13f) - GH-675 Relax validation on function arguments with default values
- [fb2bd08](http://github.com/CraterCrash/godot-orchestrator/commit/fb2bd08ab020892a0409fa6044aac4f297ca2d01) - Reset assign type on duplicate/paste
- [41269c3](http://github.com/CraterCrash/godot-orchestrator/commit/41269c313e1141578737e8622d4bf72e2a6699d7) - Improve Local Variables Initialization & Assignment UX
- [111d1c5](http://github.com/CraterCrash/godot-orchestrator/commit/111d1c5744cc392cea38e2b9f0a65b5f706d07b3) - GH-658 Fix Godot 4.2 compatibility
- [62492a2](http://github.com/CraterCrash/godot-orchestrator/commit/62492a2d7f7374d7c4354a81e3bd8eac6f50f647) - GH-658 Add version pick to plug-in updater subsystem
- [f04744c](http://github.com/CraterCrash/godot-orchestrator/commit/f04744c3620a54fa8c80eba23627819591873f8a) - Fix typo
- [75ff266](http://github.com/CraterCrash/godot-orchestrator/commit/75ff266bf250626eafcac356c3f773ead4d31486) - GH-666 Correctly handle Toggle Breakpoint keybind
- [f50568e](http://github.com/CraterCrash/godot-orchestrator/commit/f50568e60d8956a1f8e786e6fdbc3369f401c8c0) - GH-667 Relax operator node connection requirements
- [31e2f9c](http://github.com/CraterCrash/godot-orchestrator/commit/31e2f9ce5fab706d51d31c49feae7ee28c8147f4) - GH-664 Use screen space for calculating paste position
- [973f7c3](http://github.com/CraterCrash/godot-orchestrator/commit/973f7c330e12e612de8c09ac5430ea0ff5e50971) - GH-662 Exclude loop-based pins from return value check
- [6bfacdd](http://github.com/CraterCrash/godot-orchestrator/commit/6bfacdde27a550c7f4c21daba715a4cb8ee89c42) - Add release_manifests.json
- [a0f141c](http://github.com/CraterCrash/godot-orchestrator/commit/a0f141c0fa13090731634e4710883170bcc718e5) - GH-646 Only reset return value when last return node removed
- [1f92561](http://github.com/CraterCrash/godot-orchestrator/commit/1f925615da23f9952e962fdef31d9ae7b1bc29a2) - GH-646 Allow return nodes to be duplicated
- [a7c4ddb](http://github.com/CraterCrash/godot-orchestrator/commit/a7c4ddbca25d981954c8a877c6443c5a13519957) - GH-646 Function graph validation for return node links
- [0392024](http://github.com/CraterCrash/godot-orchestrator/commit/0392024f5d75ac966f39bd43abfa6cced9af44f5) - GH-651 Move build/validate output to a bottom panel
- [d9354f9](http://github.com/CraterCrash/godot-orchestrator/commit/d9354f9e86bea89d6f3c758507638b380b78e389) - GH-647 Store/Cache/Sync knots correctly on pin disconnects
- [9a5e5d5](http://github.com/CraterCrash/godot-orchestrator/commit/9a5e5d53f38d57c93299227c4d7526583384f8ed) - GH-653 Resize main toolbar icon with display scale
- [f9892fb](http://github.com/CraterCrash/godot-orchestrator/commit/f9892fbe783243bc4808dbdc3f02255f5e588c70) - Fix lambda unused capture
- [46dc432](http://github.com/CraterCrash/godot-orchestrator/commit/46dc432b56a41e26678164def29beac51eecd447) - GH-643 Allow `String` to connect to `StringName` pins
- [f29fe1b](http://github.com/CraterCrash/godot-orchestrator/commit/f29fe1bb4ee5e338f0a69d636e74fbf1c16c9299) - GH-642 Allow `Object` to connect to `Boolean` pins
- [4e514d2](http://github.com/CraterCrash/godot-orchestrator/commit/4e514d2eb25896a22b19362d3f1dca376bd73813) - GH-638 Guard against reconnection on node duplication
- [35b82cb](http://github.com/CraterCrash/godot-orchestrator/commit/35b82cb27b8df1dadedcab5798894a5f7ce8bce2) - Correctly clean-up placeholder instances
- [9fc12e9](http://github.com/CraterCrash/godot-orchestrator/commit/9fc12e9dbdb8650eb51f4b0970111abaff4624e9) - Fix editor crashes reverting inspector values for constant nodes
- [6e33e12](http://github.com/CraterCrash/godot-orchestrator/commit/6e33e12c5a3a35c5c942bc8f83f3594542632bd6) - GH-632 Add new function call node color variants
- [beb964c](http://github.com/CraterCrash/godot-orchestrator/commit/beb964c3c1fe2a528a3602b62cfd9191a263783b) - GH-630 Add link to GH issue
- [214c184](http://github.com/CraterCrash/godot-orchestrator/commit/214c18487f5560fb1c73d1e5bbb015f98f6955ce) - GH-630 Enhance `VariableGet` with validation mode
- [26d8395](http://github.com/CraterCrash/godot-orchestrator/commit/26d8395e1538ea7761c156156444aad85d1440b0) - GH-634 Protect against double callback dispatch in Godot 4.2.x
- [832ac92](http://github.com/CraterCrash/godot-orchestrator/commit/832ac92b961882e080f0068d963a2899fc1a2e95) - GH-628 Synchronize signal slot disconnect UI updates
- [10e8a74](http://github.com/CraterCrash/godot-orchestrator/commit/10e8a74b893245c5bd07fd08c2378c60aeed23b7) - GH-626 Fix handling of script placeholder properties

## [2.1.rc3](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.1.rc3) - 2024-07-29

- [1e12d00](http://github.com/CraterCrash/godot-orchestrator/commit/1e12d009c0af686f74968fde6ecd1d176c176ac9) - Update about dialog details
- [ac097d0](http://github.com/CraterCrash/godot-orchestrator/commit/ac097d0be937b2d91606354fbe72ff3c967410dd) - GH-622 Revert commit 8f15816b092c9a92b878db319a496430925f8816
- [5388fa3](http://github.com/CraterCrash/godot-orchestrator/commit/5388fa35b8ed5d59f30196a408a26839f25bd7b8) - GH-595 Improve initial rendering of variable type selection
- [51947b2](http://github.com/CraterCrash/godot-orchestrator/commit/51947b2ea98dc8ad6e23587f591db4028f8589bb) - GH-610 Add in-editor script node documentation
- [bfd94ff](http://github.com/CraterCrash/godot-orchestrator/commit/bfd94ff17c6468445b27878ebd3a0e9a5a2562d7) - GH-607 Improve changing variable types
- [129ce72](http://github.com/CraterCrash/godot-orchestrator/commit/129ce72aa3024fd7d620984f8000897c2f1f47de) - GH-617 Retain `PrintString` defaults across node refreshes
- [97e1ed1](http://github.com/CraterCrash/godot-orchestrator/commit/97e1ed1b458018b888fc654422d6193e5b619d18) - GH-612 Fix view documentation not rendering
- [d089784](http://github.com/CraterCrash/godot-orchestrator/commit/d089784055580b4b656cc4155ce278012a14b4f5) - GH-614 Mark several classes as internal
- [3fefdbb](http://github.com/CraterCrash/godot-orchestrator/commit/3fefdbb2db6e384ac8ce31618aa1405b66eb603a) - GH-605 Improve numeric default value widget validation
- [5c4babd](http://github.com/CraterCrash/godot-orchestrator/commit/5c4babd81727d74ad0f96e05fbbac77c37cadae4) - GH-596 Render `int`, `float`, and `bool` with user-friendly names
- [26b28b1](http://github.com/CraterCrash/godot-orchestrator/commit/26b28b1a5cf781b3124b380d328de1ba89fc20ca) - GH-592 Add optional delete node confirmation dialog
- [b7e2622](http://github.com/CraterCrash/godot-orchestrator/commit/b7e26221a0a93376dd1b0c454c8c8a50ab054586) - GH-593 Fix collapse to function bugs
- [b6f8ddc](http://github.com/CraterCrash/godot-orchestrator/commit/b6f8ddce8254c79efae3f0280dc83daa1bcd47cd) - GH-603 Permit selecting abstract types for variable definitions
- [f1cda9f](http://github.com/CraterCrash/godot-orchestrator/commit/f1cda9f164582a0b09a5c986684de72221c9eb0b) - Remove random UtilityFunction::print call
- [055870b](http://github.com/CraterCrash/godot-orchestrator/commit/055870b251fff93db832980faf49b7e3d468379f) - GH-590 Use our own instance of ScriptCreateDialog
- [bee5a48](http://github.com/CraterCrash/godot-orchestrator/commit/bee5a48c4c5e4e833fced578a832fe1e3b727f5e) - Fix lambda capture warnings
- [aed1d08](http://github.com/CraterCrash/godot-orchestrator/commit/aed1d08bae4cf890eb12e6dbbf2696cdb2acbfaa) - Fix some signed/unsigned build warnings
- [679c0f6](http://github.com/CraterCrash/godot-orchestrator/commit/679c0f6bfba93f908cefce3697bad12ce33cde7f) - GH-584 Disable knot creation inside node rect
- [ebbdfff](http://github.com/CraterCrash/godot-orchestrator/commit/ebbdfff590b60026efc1dc63cb03b3dcd7cd6c44) - GH-574 Strengthen validation for adding/renaming component items
- [8dd4e2f](http://github.com/CraterCrash/godot-orchestrator/commit/8dd4e2f59bd14179ca9c421cc390d43ef2b8320f) - GH-582 Auto-select first action when node first placed
- [8f15816](http://github.com/CraterCrash/godot-orchestrator/commit/8f15816b092c9a92b878db319a496430925f8816) - GH-583 Always show execution pins on user-defined script functions
- [7c8f566](http://github.com/CraterCrash/godot-orchestrator/commit/7c8f5660837d0e40c29038ff3ba3ed22be04473e) - GH-578 Make debugger errors more clear
- [b9ea73f](http://github.com/CraterCrash/godot-orchestrator/commit/b9ea73f34590eb2a57920835b49580f3d9aefb1e) - GH-575 Correctly focus script on editor breakpoints
- [98fdb36](http://github.com/CraterCrash/godot-orchestrator/commit/98fdb36ae7d5d1de95a1e072e3d21ee5f1665c00) - GH-579 Raise breakpoint when input action has no action name

## [2.1.rc2](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.1.rc2) - 2024-07-21

- [5f9f3f4](http://github.com/CraterCrash/godot-orchestrator/commit/5f9f3f4fbb4a1561e57df5cde982d247aa38161c) - GH-567 Correctly validate local variable node connection
- [6647c3a](http://github.com/CraterCrash/godot-orchestrator/commit/6647c3af4d3249d21d90cac200f81a26fd8f98fd) - GH-568 Relax property validation

## [2.1.rc1](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.1.rc1) - 2024-07-21

- [53c628a](http://github.com/CraterCrash/godot-orchestrator/commit/53c628ad2b8a18eccbbc1ce5cd82187042285ca2) - GH-563 Relax validation on function result node
- [8d50ade](http://github.com/CraterCrash/godot-orchestrator/commit/8d50ade7516e6e89a143e6f2a59e44a657042d7b) - GH-560 Add global script filename to search results
- [f263523](http://github.com/CraterCrash/godot-orchestrator/commit/f263523340cfb4e0abbed514bf7db33b8a314cb3) - GH-555 Add custom icons for pass-by reference/value
- [922ac98](http://github.com/CraterCrash/godot-orchestrator/commit/922ac9880afa5195d4dcf07ab9a2880b20f80cf7) - GH-555 Add pass-by- value/reference to property container UI
- [a1f3f1a](http://github.com/CraterCrash/godot-orchestrator/commit/a1f3f1aff7767f37b7ec32f599b36d2f95d3a8c6) - GH-559 Fix `text_submitted` due to multiple registration calls
- [b52afce](http://github.com/CraterCrash/godot-orchestrator/commit/b52afceb24ff873a06e1e6be7ac7807c9c95c87c) - Fix API compatibility with Godot 4.2.1
- [796a680](http://github.com/CraterCrash/godot-orchestrator/commit/796a6802d7c049dfc39002a2aece071b880542f8) - Fix build errors
- [c9df854](http://github.com/CraterCrash/godot-orchestrator/commit/c9df854fbc40c636a8ffc3f8a0c90ac5329c8efe) - GH-556 Align variable type plugin with new search type
- [6147e28](http://github.com/CraterCrash/godot-orchestrator/commit/6147e280bfe0c4ac93974cd2c669b3a5a2993d50) - GH-556 Generalize "select type" dialog
- [1768f9a](http://github.com/CraterCrash/godot-orchestrator/commit/1768f9a735e57659ee346215fb1a7fdf5d407aea) - GH-551 Avoid duplicating return nodes on rename
- [d50b28c](http://github.com/CraterCrash/godot-orchestrator/commit/d50b28cf8ce2261862f8c65b0657c45867dad260) - GH-497 Function/Signals support all class/argument types
- [d3ba7c9](http://github.com/CraterCrash/godot-orchestrator/commit/d3ba7c9c5dffc8552d56eddf1b6002c06d9e6ad2) - GH-549 Add additional keywords for ComposeFrom/Decompose nodes
- [88d2df0](http://github.com/CraterCrash/godot-orchestrator/commit/88d2df06d4c64b3683d7af31ca30286c07c9ad75) - GH-547 Duplicate node instances on paste
- [77466fe](http://github.com/CraterCrash/godot-orchestrator/commit/77466febb878cb31224c7f40a2acd8f878bbbeac) - GH-338 Support variables as script global class types
- [ab763ab](http://github.com/CraterCrash/godot-orchestrator/commit/ab763ab39ae83d20504e51683d3a49627f311e63) - GH-534 Correctly resolve self for NodePath popups
- [a04a9d9](http://github.com/CraterCrash/godot-orchestrator/commit/a04a9d9d785dba4b61171056a2ff979ed60d08c7) - GH-532 Fix Godot 4.2.1 compatibility
- [43379ec](http://github.com/CraterCrash/godot-orchestrator/commit/43379ecce7ceb2ad2baedaf9f9c791e9119877d0) - GH-532 Align Get Scene Node class on path change
- [674fb74](http://github.com/CraterCrash/godot-orchestrator/commit/674fb7480378bb660bbe4416e4c0f10921ac020b) - GH-537 Fix Godot 4.2.1 compatibility
- [c011903](http://github.com/CraterCrash/godot-orchestrator/commit/c0119037d32b3122a93e884558669bccc1f369b2) - GH-537 Fix target resolution
- [dbced0d](http://github.com/CraterCrash/godot-orchestrator/commit/dbced0dedcb1dbd68875ba215b284aa802f50e4f) - GH-542 Fix tween animation for focusing graph nodes
- [5f6de0e](http://github.com/CraterCrash/godot-orchestrator/commit/5f6de0e698db7c4e58f7d204461a606c20f1517d) - GH-533 Ignore default value widgets on target pins
- [7df2f18](http://github.com/CraterCrash/godot-orchestrator/commit/7df2f18d79ac813360480010457c1eb65bbde699) - Add Mac, Linux, and Web ignores to .gitignore file
- [1230949](http://github.com/CraterCrash/godot-orchestrator/commit/1230949ede0ce318f0cc113e208c65dbae9f48e4) - GH-539 Fix placeholder instance clean-up

## [2.1.dev4](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.1.dev4) - 2024-07-15

- [b642c29](http://github.com/CraterCrash/godot-orchestrator/commit/b642c29884fe472dc38110c55fcc0b8aad46ed91) - GH-529 Improve add component item workflow
- [3e4985e](http://github.com/CraterCrash/godot-orchestrator/commit/3e4985efd38b42f41688012234c730b698c255ed) - GH-527 Improve all action search/category classifications
- [0edcefa](http://github.com/CraterCrash/godot-orchestrator/commit/0edcefaeafd3dc3393d16198aa76cc6c52a06ee4) - GH-59 Add configurable corner/border radius
- [0ba992d](http://github.com/CraterCrash/godot-orchestrator/commit/0ba992d7a24ea1c1165ef2616dd0d1b1b5a52494) - GH-521 Fix compatibility issue with Godot 4.2.1 and `set_auto_translate_mode`
- [3d99bc5](http://github.com/CraterCrash/godot-orchestrator/commit/3d99bc5b2f7b54d2a93886fbd6a486c0b9408f81) - GH-521 Fix compatibility issue with Godot 4.2.1 and `containsn`
- [831a41b](http://github.com/CraterCrash/godot-orchestrator/commit/831a41bc306c4707fd6f19d1fc01fee0b76e8ddd) - Add instanced scene indicator to scene node selector
- [234cd19](http://github.com/CraterCrash/godot-orchestrator/commit/234cd19db366b7e5555cfec74f961fb8ad49ba41) - GH-472 Show event actions when dragging from input pins
- [b2a7206](http://github.com/CraterCrash/godot-orchestrator/commit/b2a720647aa6faa45ef9cfc3ad50553af68bf2df) - GH-473 Improve view documentation jump for nodes
- [de1e4b5](http://github.com/CraterCrash/godot-orchestrator/commit/de1e4b5f05af7fe65f3304cbaa9014488e6c676a) - Update .gitignore
- [f99c6ea](http://github.com/CraterCrash/godot-orchestrator/commit/f99c6eabe809749b56704431e460b02b083006c1) - GH-498 Improve NodePath dialogs for nodes and properties
- [990e3dd](http://github.com/CraterCrash/godot-orchestrator/commit/990e3dd7f2e17ef39471ed48eb1f78c9a6b495c1) - GH-491 Improve graph node left/right column rendering
- [9cb1179](http://github.com/CraterCrash/godot-orchestrator/commit/9cb11794849f123f33c323a347cb6726bd5e5555) - GH-496 Track editor graph state in editor cache
- [5dbd06c](http://github.com/CraterCrash/godot-orchestrator/commit/5dbd06cf91149785d40ee66c094c13a711dfb3ce) - GH-512 Correctly resolve target object
- [0b67f50](http://github.com/CraterCrash/godot-orchestrator/commit/0b67f5010d6478e304bd331fbf5709c94362d66c) - GH-517 Run orphan fix-up during Orchestration post-initialize
- [a4275f5](http://github.com/CraterCrash/godot-orchestrator/commit/a4275f513b8e2742d7be8ff73ff44040e28bed33) - GH-511 Add label to Enum/Bitfield return function values
- [30b4a57](http://github.com/CraterCrash/godot-orchestrator/commit/30b4a570b4e9894054fc129434ad2b272aedca5c) - GH-510 Pass class type initialization to New Object nodes
- [f6dd912](http://github.com/CraterCrash/godot-orchestrator/commit/f6dd9128b744b886b6bef9308998d0299f32a7ed) - GH-505 Make node context menu items multi-select aware
- [160d8db](http://github.com/CraterCrash/godot-orchestrator/commit/160d8dbcc572ff0e8902aab34a55730221e24498) - GH-490 Make PopupMenu instances dynamic for Node/Pins
- [e671c01](http://github.com/CraterCrash/godot-orchestrator/commit/e671c0124c5f119ad1468d6b748ceabcda25ae5c) - GH-502 Fix action menu item alignment
- [54c7fde](http://github.com/CraterCrash/godot-orchestrator/commit/54c7fde8bc0b3f7240f8a268ba692e9497e8f2bd) - GH-470 Operator node - always autowire first eligible pin
- [fd31f74](http://github.com/CraterCrash/godot-orchestrator/commit/fd31f74b4b67c4d793535c11706fb768414eab29) - GH-488 Update InspectorDock when ProjectSettings change
- [2b12f63](http://github.com/CraterCrash/godot-orchestrator/commit/2b12f6328db43b537ae05830a18ef669be289a0f) - Add a boolean setting to show the 'arrange' button in the UI.
- [f7e000a](http://github.com/CraterCrash/godot-orchestrator/commit/f7e000a6bb00b1a6dde6bc890013671e4036b6b1) - GH-487 Changing function argument type resets default value
- [9dfde8a](http://github.com/CraterCrash/godot-orchestrator/commit/9dfde8a77f3271164fb36e5c20b0821588b0f47d) - GH-444 Fix potential nullptr exception
- [e38942d](http://github.com/CraterCrash/godot-orchestrator/commit/e38942df8ea7be344b23525c597c660714c16ba4) - GH-444 Add compatibility for Godot 4.2
- [a4f079b](http://github.com/CraterCrash/godot-orchestrator/commit/a4f079b73b59f4373e4073279c0f95ddda8e03d5) - GH-444 Update demo orchestrations to format 2
- [b83ba9f](http://github.com/CraterCrash/godot-orchestrator/commit/b83ba9f9fdb42059f5b98249e87c3441c8b1aa8f) - GH-444 Improve pin type safety & validation
- [13093f5](http://github.com/CraterCrash/godot-orchestrator/commit/13093f53b8b9a07e01d66da296d790151890c304) - GH-444 Fix variable type resolution
- [6096533](http://github.com/CraterCrash/godot-orchestrator/commit/609653362f299c267d2ff233e416b608effd5160) - GH-444 Correctly identify variant arguments for functions/signals
- [7ef6caa](http://github.com/CraterCrash/godot-orchestrator/commit/7ef6caa28439f84fb32708b6a1a50cf6ed250e9b) - GH-444 Use helper methods for pin flags
- [283e25b](http://github.com/CraterCrash/godot-orchestrator/commit/283e25b870f4c58c4ee5bf521f7334b96165422f) - GH-486 Add connection color mapping for PackedVector4Array
- [5b99f04](http://github.com/CraterCrash/godot-orchestrator/commit/5b99f04807640b9755dccbe35f5966864d30e687) - GH-492 Allow disconnect slot from component panels
- [d185cda](http://github.com/CraterCrash/godot-orchestrator/commit/d185cda8d2d1c422a33c4cd3a1eac3bce48c9ad0) - GH-492 Correctly resolve signal argument types
- [ac5e2a2](http://github.com/CraterCrash/godot-orchestrator/commit/ac5e2a219b98cc62f4865dc458d03857baf44750) - GH-484 Show enum, bitfield, class icons in variable panel
- [80ed037](http://github.com/CraterCrash/godot-orchestrator/commit/80ed037f552e13162e3a2d4d835b9e2e75d38f66) - GH-482 Exclude internal properties from action menu
- [2d5a2dd](http://github.com/CraterCrash/godot-orchestrator/commit/2d5a2ddbab5057c21115c4f20f3d1ee88096124e) - Fix typo in build log warning message
- [9c34a96](http://github.com/CraterCrash/godot-orchestrator/commit/9c34a96922d7bfd4185544a919e9d1261ec8d230) - GH-477 Fix ExtensionDB generator
- [efb7131](http://github.com/CraterCrash/godot-orchestrator/commit/efb7131326b53bcb596042b476989bbcb59e3c83) - GH-476 Fix error handling in virtual machine
- [60e191c](http://github.com/CraterCrash/godot-orchestrator/commit/60e191c7f4c730c24d7132c3b61f511a0171b0a7) - GH-475 Fix variable property info init for class types
- [cacb2ca](http://github.com/CraterCrash/godot-orchestrator/commit/cacb2caa5378d0e2aba477c9e0dc42f63e885564) - GH-465 Apply "Refresh Nodes" option to all selected nodes
- [85b6500](http://github.com/CraterCrash/godot-orchestrator/commit/85b65006748431fcca3b4811f2868f118851b03a) - GH-452 Render knots snapped to the grid
- [80b2b98](http://github.com/CraterCrash/godot-orchestrator/commit/80b2b98f735ba7d9e6d13805f6fa77d2c94a0b11) - GH-285 Add EditorDebugger support (Requires Godot 4.3)
- [cc8e799](http://github.com/CraterCrash/godot-orchestrator/commit/cc8e7990f30f4f157fd785399d1d2a2dd0cb147a) - GH-421 Add Godot compatibility to README.md
- [8edae09](http://github.com/CraterCrash/godot-orchestrator/commit/8edae0900fd9b3fb5e88a9427ef8e05a7fe9cbc2) - GH-421 Add missing guards
- [72519a6](http://github.com/CraterCrash/godot-orchestrator/commit/72519a6f380eafcada6001c5615e70f8e27553f2) - GH-421 Bump .gdextension compatibility to 4.3
- [9e61df0](http://github.com/CraterCrash/godot-orchestrator/commit/9e61df023df95caf43940ffbce7e8f3da566b82d) - GH-421 Fix 4.3 stack allocation crash issues with RefCounted
- [764ea3f](http://github.com/CraterCrash/godot-orchestrator/commit/764ea3fec345318da1cbb2a0fcea3be5daa9de2e) - GH-421 Use `Resource::generate_scene_unique_id` instead
- [2bbd305](http://github.com/CraterCrash/godot-orchestrator/commit/2bbd305524347053208f74acb9d141ddfe61a92e) - GH-421 Add compile-time support for Godot 4.3.beta2+
- [c4f1af3](http://github.com/CraterCrash/godot-orchestrator/commit/c4f1af3f6aee94552fdf0772ecc42e0daf191680) - GH-421 Update ExtensionDB for Godot 4.3.beta2
- [8a00a61](http://github.com/CraterCrash/godot-orchestrator/commit/8a00a611e200b2dcca60ca296ca2d15fddfac416) - GH-421 Bump godot-engine/godot-cpp to 4.3.beta2
- [c8f57cc](http://github.com/CraterCrash/godot-orchestrator/commit/c8f57ccde2ac4a9b335660373bbd660d5bd751dd) - GH-460 Avoid variable panel exponential updates
- [fe17684](http://github.com/CraterCrash/godot-orchestrator/commit/fe17684ccadeb95c28067823e2e9dff710c1a77c) - GH-457 Introduce custom `FileDialog` implementation
- [b3144b7](http://github.com/CraterCrash/godot-orchestrator/commit/b3144b7b7d973c41237221e3619a06cb88c55a97) - GH-453 Fix create knot GraphNode index out of bounds error
- [563991b](http://github.com/CraterCrash/godot-orchestrator/commit/563991b4cb9d74d1fa69d29f8994be5d6e27f578) - GH-455 Fix collapse/expand fault in all actions menu
- [17fcc90](http://github.com/CraterCrash/godot-orchestrator/commit/17fcc903e898c5c6d07a6b669a22fff517111e46) - GH-263 Add default initializer values
- [d135caa](http://github.com/CraterCrash/godot-orchestrator/commit/d135caa1c4685f207996f0a68b2231255f43302e) - Refactor main layout & viewports
- [523adae](http://github.com/CraterCrash/godot-orchestrator/commit/523adae56f61af4228b5c87f6ccc529691736ebc) - Fix switch warning
- [7b7bfb2](http://github.com/CraterCrash/godot-orchestrator/commit/7b7bfb27a911a8cf162e8f663d8c0392ef055a65) - GH-263 Update demo to use text-based orchestrations
- [5f954d1](http://github.com/CraterCrash/godot-orchestrator/commit/5f954d135b00fd6294c1deefae6126d46a493432) - GH-263 Support text-based Orchestration format
- [d9734d8](http://github.com/CraterCrash/godot-orchestrator/commit/d9734d8bec4145176e75c4a28605db67d3456b14) - Add common macros header
- [e353986](http://github.com/CraterCrash/godot-orchestrator/commit/e35398655996044e6da052c8dd006f0e59aeaf2a) - Fix logical operator behavior
- [3e15c31](http://github.com/CraterCrash/godot-orchestrator/commit/3e15c31c8e2cfbeba1c8ac54566636cefffe112d) - Fix constructor init reorder warnings
- [4001860](http://github.com/CraterCrash/godot-orchestrator/commit/4001860495869ab7b38e370dbcbcdce53ce630bc) - Fix signed/unsigned comparison
- [f3fb7c0](http://github.com/CraterCrash/godot-orchestrator/commit/f3fb7c0cb4d44b0dd495aab9d63c510852625f2a) - Wrap error macro with braces
- [e8b49a2](http://github.com/CraterCrash/godot-orchestrator/commit/e8b49a208aee34b08fc9a692b6d43b47dab702a8) - Fix logical operator
- [1dfc31f](http://github.com/CraterCrash/godot-orchestrator/commit/1dfc31f781b7c3bbdf82431ba801c510a2ec0d8b) - [ci] Bump robinraju/release-downloader 1.10 to 1.11
- [36c6c53](http://github.com/CraterCrash/godot-orchestrator/commit/36c6c53d3b1aa9ba522821324b0ec900d77d3f9f) - GH-428 Fix function return types that are enum/flags
- [c626ac3](http://github.com/CraterCrash/godot-orchestrator/commit/c626ac3e9c4363fa73e6e35d94c29559362e21c4) - GH-438 Do not show validation success dialog on play game/scene
- [b788d85](http://github.com/CraterCrash/godot-orchestrator/commit/b788d85da569770ec47929563d16b37a742f8f43) - GH-416 Support instantiation of script-based classes
- [d120dad](http://github.com/CraterCrash/godot-orchestrator/commit/d120dad7f1ea036e6f0719e09fef4f50d9ebb8e7) - GH-423 Avoid reuse of default value instances
- [895a747](http://github.com/CraterCrash/godot-orchestrator/commit/895a74717263430219554695c5c3d50c5b540812) - GH-431 Upgrade of For w/Break nodes autowires aborted
- [187d764](http://github.com/CraterCrash/godot-orchestrator/commit/187d764d40837c5662c800322247e7c256d1c6fb) - GH-429 Fix compatibility with godot-minimal-theme 1.5.0
- [c32ec26](http://github.com/CraterCrash/godot-orchestrator/commit/c32ec26a6a00c2834143a651378adf4c6f01180c) - GH-434 Display validation results on success
- [63c3e71](http://github.com/CraterCrash/godot-orchestrator/commit/63c3e71301f8ca8e40e6bfbcee79a0c2bd993f8e) - Add fixup to validate button for orphaned nodes/connections
- [925d30b](http://github.com/CraterCrash/godot-orchestrator/commit/925d30bc91c648b0662fe779c520179e5d4febde) - GH-425 Fix upgrade bug with "For with Break" nodes
- [e3359d2](http://github.com/CraterCrash/godot-orchestrator/commit/e3359d23395f049732ad9ee4eebe8be8758f85be) - Add/Align usage of static `_bind_methods` in node classes
- [855fbb4](http://github.com/CraterCrash/godot-orchestrator/commit/855fbb44f7bc76352a54fbd4667671832963dfad) - GH-255 Apply property usage fixup to functions/signals
- [a64ee64](http://github.com/CraterCrash/godot-orchestrator/commit/a64ee645ed722dd8444096cdcc5491c814103c13) - GH-410 Enable friendly names by default
- [6e01d05](http://github.com/CraterCrash/godot-orchestrator/commit/6e01d057773a96533d08654ba84bdbf4b0ccb912) - GH-410 Add friendly names for Graph/Function component panels
- [74a89ef](http://github.com/CraterCrash/godot-orchestrator/commit/74a89ef35a2cc010fcc878e9ba99845bd12db648) - GH-418 Fix toolbar update when base type changes
- [572e012](http://github.com/CraterCrash/godot-orchestrator/commit/572e01254502aa20fe899f3f57f4bb8344fb6d25) - GH-414 Fix method exclusion logic to allow virtual methods

## [2.1.dev3](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.1.dev3) - 2024-06-16

- [3394eb5](http://github.com/CraterCrash/godot-orchestrator/commit/3394eb514e903c5a44f5d45981857b6a353a4ed2) - GH-408 Correctly handle component tree expand/collapse
- [5857499](http://github.com/CraterCrash/godot-orchestrator/commit/5857499fc95346c58051393a19817859771bc38a) - GH-405 Update variable tooltip text
- [9cbe57b](http://github.com/CraterCrash/godot-orchestrator/commit/9cbe57b27181de8d67ebb507381ee37c019d29f3) - GH-403 Remove redundant/superfluous text from node/pin names
- [39abada](http://github.com/CraterCrash/godot-orchestrator/commit/39abada76ccf1eba5886bb032e451deb14d91462) - GH-401 Update component panel when using context menu delete
- [f4e3fe7](http://github.com/CraterCrash/godot-orchestrator/commit/f4e3fe737946c0fcb62f033b406dae1bb6ec48b3) - GH-399 Spawn function return node when function has return value
- [0deffa7](http://github.com/CraterCrash/godot-orchestrator/commit/0deffa78f0bf7acbf2d0e6676660117325913043) - GH-364 Allow disabling autowire selection dialog
- [975b5e1](http://github.com/CraterCrash/godot-orchestrator/commit/975b5e1f7233c726125eff94fe35ac3e8f544c4d) - GH-364 Improved autowire with selection dialog
- [360678a](http://github.com/CraterCrash/godot-orchestrator/commit/360678ad7847662a9a7ddd29484f5a98f256bac8) - GH-366 Permit deleting event nodes like any other node
- [e9f4dc9](http://github.com/CraterCrash/godot-orchestrator/commit/e9f4dc91d66d3a077bb28ba76236133880e84a31) - GH-394 Split virtual machine from `OScriptInstance`
- [728bb81](http://github.com/CraterCrash/godot-orchestrator/commit/728bb813c88b3d5d79a60f5e1be842027c577557) - GH-392 Allow reselect in component panel item lists
- [b46ec6e](http://github.com/CraterCrash/godot-orchestrator/commit/b46ec6e1cd2a20de2ba23fb9a225cff646eaa1d0) - GH-365 Add disconnect control flow on drag option
- [d2c3e05](http://github.com/CraterCrash/godot-orchestrator/commit/d2c3e05e4ea76d7bf24f19b5bd1d3fd2445dffe4) - GH-379 Component panels honor editor theme
- [394f7e7](http://github.com/CraterCrash/godot-orchestrator/commit/394f7e7eaf7757ff16568aed4579ab738386eb86) - GH-390 Fix compatibility with the custom Godot minimal theme
- [fd4e21b](http://github.com/CraterCrash/godot-orchestrator/commit/fd4e21b2c5f8b8eb41bd330946ad86d641a172e1) - GH-382 Avoid recent history duplicates
- [11202e5](http://github.com/CraterCrash/godot-orchestrator/commit/11202e500ae9fa9b785e7d242301ccf53e0e2c87) - GH-385 Fix instantiate scene to respect input pin
- [a12f6b6](http://github.com/CraterCrash/godot-orchestrator/commit/a12f6b685c1be4810bf69b136777371da9d892e1) - GH-383 Split out `Orchestration` contract and refactors
- [3f2f461](http://github.com/CraterCrash/godot-orchestrator/commit/3f2f46186cae84d38379671a466027a42041a14e) - GH-360 Select action names from drop-down list
- [1b73261](http://github.com/CraterCrash/godot-orchestrator/commit/1b73261073c5ff708ebece687e04be13f7f74ddd) - GH-371 Fix zoom factor connection knot rendering
- [8f8e908](http://github.com/CraterCrash/godot-orchestrator/commit/8f8e90815d13b79c8d929677d69c2bfa4a3bde17) - Fix several code warnings
- [69b99ab](http://github.com/CraterCrash/godot-orchestrator/commit/69b99ab55194a79fa941bc776868e6c24e222a20) - [ci] Fix ubuntu builds
- [536fe94](http://github.com/CraterCrash/godot-orchestrator/commit/536fe94615e1a44856ab413dd578fd377fbd5e3d) - GH-356 Fix knot rendering on Godot 4.2
- [dfb68ae](http://github.com/CraterCrash/godot-orchestrator/commit/dfb68ae24da9d379a8c37f6faf9c10eb44bde94e) - GH-352 Curvature only between first two and last two points
- [ca11421](http://github.com/CraterCrash/godot-orchestrator/commit/ca11421f702b3790133307760c60643ec7db00de) - GH-350 Add for-each/for-loop abort output pin

## [2.1.dev2](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.1.dev2) - 2024-05-12

- [0aada9d](http://github.com/CraterCrash/godot-orchestrator/commit/0aada9d8bb379748f9e73745c11bb94ff778f035) - GH-78 Support graph connection wire knots
- [aa7daa0](http://github.com/CraterCrash/godot-orchestrator/commit/aa7daa021156599ddc1635b128a05ab26a8a8e99) - GH-347 Improve base type indicator on graph toolbar
- [e950b60](http://github.com/CraterCrash/godot-orchestrator/commit/e950b609c2a8f5789090cfda17dc2078cc0e8799) - Remove unused assignment
- [50fcd4d](http://github.com/CraterCrash/godot-orchestrator/commit/50fcd4dfc43b011b7bb968de4a508adde55d1b72) - GH-156 Collapse nodes to function and expand node feature
- [234b209](http://github.com/CraterCrash/godot-orchestrator/commit/234b209bd166e2d5081326951438862cc3ef4a87) - GH-339 Retain/store variable type filter across popup/restarts
- [78a75fe](http://github.com/CraterCrash/godot-orchestrator/commit/78a75fea206861931146e9e5ae224633b6e5a21c) - GH-345 Fix regression
- [6eb9d2a](http://github.com/CraterCrash/godot-orchestrator/commit/6eb9d2ab82f41dd2af334e4d82bf5fc786c705a1) - GH-343 Improve node styles, fidelity, and usability
- [677a920](http://github.com/CraterCrash/godot-orchestrator/commit/677a920b95fe51c5bd727da91654ddf2c1a3e232) - GH-345 Add landing page to main view
- [44c7055](http://github.com/CraterCrash/godot-orchestrator/commit/44c7055c85b06040addcdb453d4691a8e2ac417e) - GH-301 Support class/enum/bitfield based variables
- [d7df658](http://github.com/CraterCrash/godot-orchestrator/commit/d7df658a4113e0acb278a9e37cbdad54d867d16c) - GH-242 Add impl for ScriptLanguageExtension::_preferred_file_name_casing
- [49b21a5](http://github.com/CraterCrash/godot-orchestrator/commit/49b21a5fbe218f2e44accfeaa813a1f8b7f368e4) - GH-221 View doc jump to class/constant/method Godot doc
- [3896d22](http://github.com/CraterCrash/godot-orchestrator/commit/3896d22be7e9934f70270a7f2addabff7ddd106c) - GH-330 Fix Self node broken icon
- [340c512](http://github.com/CraterCrash/godot-orchestrator/commit/340c512a946e40df799bb9a2bcda3dbf7766d388) - GH-333 Implement GridPattern support for Godot 4.3
- [692d3b2](http://github.com/CraterCrash/godot-orchestrator/commit/692d3b2217a479ec65f5868f9a1d0cce59f3170c) - GH-76 Add scroll to item signal
- [d0c5e5d](http://github.com/CraterCrash/godot-orchestrator/commit/d0c5e5d5049056089d5dbfdcf5f926d5c8f25396) - GH-327 Fix editor crash on invalid reference
- [671ba65](http://github.com/CraterCrash/godot-orchestrator/commit/671ba65e26c65aca7161864abb6229ad9d1cd992) - [ci] Fix workflow
- [ead3e5f](http://github.com/CraterCrash/godot-orchestrator/commit/ead3e5f72a09318edf7fe075ce5ced2f7e0528ba) - [ci] Add arguments to change-log workflow
- [b9317fe](http://github.com/CraterCrash/godot-orchestrator/commit/b9317fed34778e93b965cbe558a17f1d74b98928) - GH-284 Add support for static functions
- [9a97c2a](http://github.com/CraterCrash/godot-orchestrator/commit/9a97c2ab42a7dab32646c8047723eb354b399750) - GH-321 Persist `For Each` node's "with break" status
- [1f82cf0](http://github.com/CraterCrash/godot-orchestrator/commit/1f82cf0efcd352fe360a1eede7c06687f4c5e28f) - GH-323 Fix persisting enum default value choices
- [c0289a7](http://github.com/CraterCrash/godot-orchestrator/commit/c0289a7af4c03e984cc885e14b1289d2ab397829) - GH-315 Have `For Each` set Element as `Any`
- [79e3a90](http://github.com/CraterCrash/godot-orchestrator/commit/79e3a908afdc5df24d85665316eee7aac6126680) - GH-318 Add `Dictionary Set` node
- [2d45ebc](http://github.com/CraterCrash/godot-orchestrator/commit/2d45ebc78ba41a413e96267646d9562b4a3860f7) - GH-313 Add Godot version compile-time constant
- [5f06fc6](http://github.com/CraterCrash/godot-orchestrator/commit/5f06fc6deb62bb49757f3e7c38307397a209f593) - GH-304 Track recently accessed orchestrations
- [7621afe](http://github.com/CraterCrash/godot-orchestrator/commit/7621afee3fc70e2b3e849d3d042978ab052f4a31) - GH-310 Support file/folder removed and files moved signals
- [cf3a2f7](http://github.com/CraterCrash/godot-orchestrator/commit/cf3a2f714bdc37a089543683aea6d8711519a235) - GH-306 Refactor `MethodUtils`
- [75726ee](http://github.com/CraterCrash/godot-orchestrator/commit/75726ee59bb05798fdd711d5d26fc1609b87d2d1) - GH-306 Fix functions to return Any types
- [07c3305](http://github.com/CraterCrash/godot-orchestrator/commit/07c3305d5c35bb8cb0d75210001f650cace7f9f6) - GH-287 Always call "Init" event if its wired
- [43f7f48](http://github.com/CraterCrash/godot-orchestrator/commit/43f7f48e1f06a0dffaa1ca12ef6758afbc937e83) - GH-283 Support method chaining
- [766bc0a](http://github.com/CraterCrash/godot-orchestrator/commit/766bc0a94bfc64c0d3214669810e19bd1b7db0c1) - GH-289 Fix built-in methods using PROPERTY_USAGE_NIL_IS_VARIANT
- [0ea31d1](http://github.com/CraterCrash/godot-orchestrator/commit/0ea31d1db861720d22b1a687464f3c7244cb7b41) - GH-282 Explicitly filter `_get` and `_set` methods
- [35a6b7e](http://github.com/CraterCrash/godot-orchestrator/commit/35a6b7ebc7d9090ebcf09bbf04ec7ea1ff05eb21) - GH-297 Preserve setting order when setting is modified
- [037ec30](http://github.com/CraterCrash/godot-orchestrator/commit/037ec302e0265b9ae4afd98de21e9d4cf3750d45) - Fix comparison signed/unsigned mismatch
- [229e723](http://github.com/CraterCrash/godot-orchestrator/commit/229e7238afdd9a701ddf8a602489171aacc69ace) - GH-272 Remove obsolete documentation
- [7602de2](http://github.com/CraterCrash/godot-orchestrator/commit/7602de28930113fed0133fc5ac5b8158dcb8f3ff) - Rework editor icon lookups

## [2.1.dev1](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.1.dev1) - 2024-04-22

- [3508d11](http://github.com/CraterCrash/godot-orchestrator/commit/3508d1170fb363802ac71e6895e1b0a6fa26fb57) - GH-206 Correctly generates action items for instantiated scenes
- [d58ca8a](http://github.com/CraterCrash/godot-orchestrator/commit/d58ca8ab2a4c2371445ad47b3ce55a9f0cee437e) - GH-205 Dim wires when using `highlight_selected_connections`
- [7ee7f94](http://github.com/CraterCrash/godot-orchestrator/commit/7ee7f941ed1a1ad4a17a72eeb594b8933957e8b8) - GH-248 Construct `Callable` with correct target reference
- [16d5503](http://github.com/CraterCrash/godot-orchestrator/commit/16d55033aefbaf0388b0a4e52e9ada6af3e9944d) - GH-273 Force `PrintStringUI` to ignore mouse events
- [a64f593](http://github.com/CraterCrash/godot-orchestrator/commit/a64f59326a702bebcea8214330133c0785610482) - GH-254 Fix emitting non-Orchestrator based signals
- [c18f68a](http://github.com/CraterCrash/godot-orchestrator/commit/c18f68a856138696b3c58dfeff7f3f4602c4b203) - GH-238 Rework/Improve Bitfield pin value rendering
- [0c78712](http://github.com/CraterCrash/godot-orchestrator/commit/0c787129e95299024745b7780dec2229a38c768b) - GH-265 Support Object `new()` and `free()` nodes
- [7548c45](http://github.com/CraterCrash/godot-orchestrator/commit/7548c45972114e25ad87c1964e6cb1bf12fe37d7) - GH-266 Correctly resolve property class actions
- [09dbfed](http://github.com/CraterCrash/godot-orchestrator/commit/09dbfed3dce3f8bfd58de8dd59e8a87994c049e6) - GH-253 Include registered autoloads in all actions menu
- [1523e3a](http://github.com/CraterCrash/godot-orchestrator/commit/1523e3a2f3e2b52ff36e3770f720c0dcd2648155) - GH-268 Do not serialize OScriptNode flags
- [c2c8a77](http://github.com/CraterCrash/godot-orchestrator/commit/c2c8a7712404720206b84d9d9142f6316fbb5a04) - [ci] Upgrade tj-actions/changed-files from v44.1.0 to v44.3.0
- [d639a00](http://github.com/CraterCrash/godot-orchestrator/commit/d639a00984048cf64151bfd38668d2169c5d3317) - Don't use bitwise `&` operator, use correct operator
- [af1b9fe](http://github.com/CraterCrash/godot-orchestrator/commit/af1b9fe2c1989be80b27de377ea13db7565d71eb) - Don't make unnecessary copies of OScriptGraph
- [b99eff2](http://github.com/CraterCrash/godot-orchestrator/commit/b99eff2bc4da9edd0be1a0b0c07b1c3aaa6aaed6) - [ci] Upgrade tj-actions/changed-files from v44.0.1 to v44.1.0
- [a643406](http://github.com/CraterCrash/godot-orchestrator/commit/a643406eb9033dc6d94ca5abf61ca5448f0ad820) - [ci] Android artifacts use 'lib' prefix
- [48c4179](http://github.com/CraterCrash/godot-orchestrator/commit/48c41795176fdd6c3cd687ce963bc18176c018ba) - [ci] Disable debug builds
- [65da934](http://github.com/CraterCrash/godot-orchestrator/commit/65da93480705fcb558f7dc58125e3c80748384da) - [ci] Add android build
- [61cd74d](http://github.com/CraterCrash/godot-orchestrator/commit/61cd74d5c3209ffc728fc482978c082cf358d6d7) - Update documentation link in README.md
- [5ec6e82](http://github.com/CraterCrash/godot-orchestrator/commit/5ec6e827a0e1bffcf444f46e8e1cad709dd5ca89) - [ci] Upgrade tj-actions/changed-files from v44.0.0 to v44.0.1
- [7018570](http://github.com/CraterCrash/godot-orchestrator/commit/7018570030f39e9a963a605bb1a304fa68c54d20) - [ci] Upgrade robinraju/release-downloader from v1.9 to v1.10
- [e11519f](http://github.com/CraterCrash/godot-orchestrator/commit/e11519f4595f21b4675722ae3e0dcd30c4daf350) - GH-245 Force `ExtensionDB` generator to use LF line endings
- [c563837](http://github.com/CraterCrash/godot-orchestrator/commit/c563837b86f1078bce744b14b48bf992f59e356d) - GH-239 Provide `ExtensionDB::is_class_enum_bitfield` look-up
- [112f566](http://github.com/CraterCrash/godot-orchestrator/commit/112f5666aa218fbf1ee1c147984122a50a340985) - GH-237 Render @GlobalScope and class-scope bitfields uniformly
- [633cd18](http://github.com/CraterCrash/godot-orchestrator/commit/633cd184f6a90d1567fce598e87c65494f337268) - GH-240 Standardize Method return value check
- [14bcedf](http://github.com/CraterCrash/godot-orchestrator/commit/14bcedff8786dc6e239ae6ea7fa2c3a35e578905) - GH-217 Guard against nullptr
- [937011c](http://github.com/CraterCrash/godot-orchestrator/commit/937011c80646fdcb28b279f52068453191426e16) - GH-217 Add signal connection indicator for user-defined functions
- [bcc03ed](http://github.com/CraterCrash/godot-orchestrator/commit/bcc03edbbc73585e501ad9459d28fbfb9b1d47cc) - GH-232 Remove superfluous warnings saving callable/signal types
- [3abdc3d](http://github.com/CraterCrash/godot-orchestrator/commit/3abdc3d8a613d2ead4fdbcc0685230e07d15fac5) - GH-230 Reconstruct existing node on sync request
- [5ccab43](http://github.com/CraterCrash/godot-orchestrator/commit/5ccab43539a79ebcf9727a3a4a4708a3da6a1e91) - GH-222 Allow some constant/singleton names be searchable
- [139a5f0](http://github.com/CraterCrash/godot-orchestrator/commit/139a5f0819a79674ae8f3123dfdbf530f834ca79) - GH-227 Correctly encode Godot vararg built-in functions
- [3c1900e](http://github.com/CraterCrash/godot-orchestrator/commit/3c1900ec1e91cb78fe505f513443b0cfefdb42df) - Update README screenshots
- [8d60bc8](http://github.com/CraterCrash/godot-orchestrator/commit/8d60bc8e550336c6c095b0eb097a0fa7d7d0a941) - GH-207 Change orchestration view based on active root scene node
- [57041b1](http://github.com/CraterCrash/godot-orchestrator/commit/57041b1c891de667bc8e9c5dfe1d04979f30a0cd) - GH-204 Spawn override function at center of view
- [6e69142](http://github.com/CraterCrash/godot-orchestrator/commit/6e69142ec827eb2bff004355a9b85114f5ef21c8) - GH-215 Initially set function arguments to default values

## [2.0.stable](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.0.stable) - 2024-03-30

- [280aadc](http://github.com/CraterCrash/godot-orchestrator/commit/280aadcb5c592722e52718af4bb247c249dc5a57) - GH-202 Fix disconnect error on `tab_changed` signal
- [30d6299](http://github.com/CraterCrash/godot-orchestrator/commit/30d6299a21786024e9b1669ff826f2b0b789a3fe) - GH-208 Fix patch/build version check logic
- [5038580](http://github.com/CraterCrash/godot-orchestrator/commit/503858047c6c90b28a57351aed7f6e080de96602) - GH-208 Add auto-update functionality
- [ecbdebf](http://github.com/CraterCrash/godot-orchestrator/commit/ecbdebf2fd3e96cad724c16788d47ca7e1f1fba1) - [ci] Upgrade tj-actions/changed-files from v43.0.1 to v44.0.0

## [2.0.rc2](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.0.rc2) - 2024-03-24

- [65f82f2](http://github.com/CraterCrash/godot-orchestrator/commit/65f82f2229f09fd04b35960c1954c3ba44805881) - Rework about dialog
- [3b17834](http://github.com/CraterCrash/godot-orchestrator/commit/3b1783489887777c51b1291c51dbbb2dc7d48243) - GH-176 Move dialog center prior to popup
- [10034e3](http://github.com/CraterCrash/godot-orchestrator/commit/10034e3ec8df8ef76179298752d07ff79932ff3c) - GH-193 Allow highlighting selected nodes/connections
- [1e4ae2e](http://github.com/CraterCrash/godot-orchestrator/commit/1e4ae2e6b6ab56cbef3432878bfd5fd9e2f3a0e8) - GH-183 Signal function connection indicator/dialog
- [34f85f8](http://github.com/CraterCrash/godot-orchestrator/commit/34f85f877dec17f4526ac898b2086f72f56322a3) - GH-177 Fix type propagation for Preload and Instantiate Scene
- [e769057](http://github.com/CraterCrash/godot-orchestrator/commit/e7690574fa98026435ab8d35369e62a8734ecaf7) - GH-194 Reduce title text in call function nodes
- [57e13ce](http://github.com/CraterCrash/godot-orchestrator/commit/57e13ce430ddab059df0fa4757309acc53ccc501) - GH-187 Add BitField input/default value support
- [3599beb](http://github.com/CraterCrash/godot-orchestrator/commit/3599bebde3aaab834fca4c71c58c0332e7bf5896) - GH-180 Support variadic function arguments
- [5e36291](http://github.com/CraterCrash/godot-orchestrator/commit/5e3629180135fdfffea93d0b111877ae172ff573) - GH-190 Do not cache instantiated scene node
- [fdec82f](http://github.com/CraterCrash/godot-orchestrator/commit/fdec82f958e94c2aa82010d0eace46f1f3e247dd) - GH-179 Remove Function Call `(self)` indicator from non-target pins
- [1590194](http://github.com/CraterCrash/godot-orchestrator/commit/1590194efb24248638536029b65386742ce73742) - GH-179 Function Call `Target` pin no longer autowired
- [b19db62](http://github.com/CraterCrash/godot-orchestrator/commit/b19db62f31e78495f47ebae0af2f4ca4d539ea81) - GH-182 The `ENTER` key releases node input focus
- [4e146cb](http://github.com/CraterCrash/godot-orchestrator/commit/4e146cba5eeedf8a9fb3a5abea1c3c8191c71a0e) - GH-176 Position all actions menu based on `center_on_mouse`
- [2b95ddd](http://github.com/CraterCrash/godot-orchestrator/commit/2b95ddd35423082b89b9ea621deaa516acbf0e2d) - [ci] Upgrade tj-actions/changed-files from v43.0.0 to v43.0.1
- [d84191d](http://github.com/CraterCrash/godot-orchestrator/commit/d84191dbc4282f7cb33ef19faf9cbb7b78134179) - GH-173 Align styles with non-comment nodes
- [6cd0b66](http://github.com/CraterCrash/godot-orchestrator/commit/6cd0b66aea13d6d8d9d0cca37bc5e55bd5f96f55) - GH-173 Sync comment background color on inspector change
- [df24b23](http://github.com/CraterCrash/godot-orchestrator/commit/df24b23d6e6a877a41ba07b5f15bea12d9ca0cfb) - Update README.md
- [551ed1f](http://github.com/CraterCrash/godot-orchestrator/commit/551ed1f738fe64c8c11d1a4ae37afe648198fa51) - GH-98 Distribute Orchestrator icon .import files
- [df6691f](http://github.com/CraterCrash/godot-orchestrator/commit/df6691fff827a8df9f69a16570ac1b25b4fa3eb5) - GH-152 Do not change node icons when script attached
- [269e8e9](http://github.com/CraterCrash/godot-orchestrator/commit/269e8e9f2079226bf9c33634ee1dd75999f694b5) - GH-167 Use `ubuntu-20.04` for GLIBC 2.31
- [161976e](http://github.com/CraterCrash/godot-orchestrator/commit/161976e26078cf28a0da6add0f76063110ee85c4) - [ci] Upgrade tj-actions/changed-files from v42.1.0 to v43.0.0
- [de49a48](http://github.com/CraterCrash/godot-orchestrator/commit/de49a480e4512b2095a0a39d4b2b31928b105c80) - GH-164 Add Local Variable types Object & Any
- [38e5b43](http://github.com/CraterCrash/godot-orchestrator/commit/38e5b43df3614a3b49d4d2436c568b624cc949c4) - GH-158 Correctly escape modulus title text
- [7d77cff](http://github.com/CraterCrash/godot-orchestrator/commit/7d77cffe6da374574acd20e9c619a7ec9fd04eaa) - GH-155 Allow toggling component panel visibility
- [6113a9c](http://github.com/CraterCrash/godot-orchestrator/commit/6113a9c4b84a79497effa07d74173b92df5bf7c4) - GH-159 Support all Packed-Array types
- [63011e1](http://github.com/CraterCrash/godot-orchestrator/commit/63011e1d397232321bc948298659641c75b5cea1) - [ci] Upgrade tj-actions/changed-files from v42.0.7 to v42.1.0
- [7c6731e](http://github.com/CraterCrash/godot-orchestrator/commit/7c6731e72793b774d4d7b1fe9c6b796255100f0d) - Update documentation workflow

## [2.0.rc1](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.0.rc1) - 2024-03-10

- [468efc6](http://github.com/CraterCrash/godot-orchestrator/commit/468efc63bf36ac24848a7a709fcbf40ef87cd795) - [docs] Update tutorial
- [5522fd1](http://github.com/CraterCrash/godot-orchestrator/commit/5522fd11df1dc027fc0f8b0430133d4285976fe9) - [docs] Update docs images with new styles
- [7ed918c](http://github.com/CraterCrash/godot-orchestrator/commit/7ed918c8620c7720466df92699c49b4e22d26887) - [docs] Update nodes
- [2b5ba1a](http://github.com/CraterCrash/godot-orchestrator/commit/2b5ba1a90d9c98b6d0875bdecc1ac93d6396309e) - GH-145 Use `snprintf` instead
- [cbd89b2](http://github.com/CraterCrash/godot-orchestrator/commit/cbd89b22f5bf149c72070e051a614f2ad6969db4) - GH-145 Use arch_linux compatible time formatting
- [ebb0b0b](http://github.com/CraterCrash/godot-orchestrator/commit/ebb0b0bf3b24a31f002aeac42140903ed4202e10) - [ci] Upgrade tj-actions/changed-files from v42.0.6 v42.0.7
- [2a64fe4](http://github.com/CraterCrash/godot-orchestrator/commit/2a64fe44c2a22b815d5484ef571c4adcc1dc8dee) - GH-136 Add build validation for autoload node
- [696f89a](http://github.com/CraterCrash/godot-orchestrator/commit/696f89a1f729438889bb0891e2eea792ee97db6d) - GH-136 Add support for accessing Godot autoloads
- [dfb8ece](http://github.com/CraterCrash/godot-orchestrator/commit/dfb8eceec8c5c39c5830f6ec78697e84bdb4632a) - GH-143 Return null rather than 0-sized property info list
- [290f57e](http://github.com/CraterCrash/godot-orchestrator/commit/290f57ee5406351591d632d61551d56618e1ebb1) - [ci] Upgrade tj-actions/changed-files from v42.0.5 to v42.0.6
- [573abaa](http://github.com/CraterCrash/godot-orchestrator/commit/573abaa18b2b122489999ff45e451e79d813e49b) - GH-122 Introduce `Instantiate Scene` node
- [92c7b06](http://github.com/CraterCrash/godot-orchestrator/commit/92c7b06eb62a032fe968c5fe0529a2eb7e5842a9) - GH-122 Make file pin behavior generic
- [2f84493](http://github.com/CraterCrash/godot-orchestrator/commit/2f844933ef730124636a81b3d6264466201c584c) - GH-118 Fix equality ambiguity failure
- [3a98226](http://github.com/CraterCrash/godot-orchestrator/commit/3a982264068d6a61d2da9774ad7655c1930a0a67) - GH-118 Fix NodePath property resolution
- [33a995d](http://github.com/CraterCrash/godot-orchestrator/commit/33a995ddf8ddcd03525eca56649d8f4c969acd00) - GH-118 Fix GetSceneNode resolution
- [24f9f5a](http://github.com/CraterCrash/godot-orchestrator/commit/24f9f5af46b2df43027b15c09584eaa0264c8a1d) - GH-121 Dispatch only script user-defined functions
- [19026d5](http://github.com/CraterCrash/godot-orchestrator/commit/19026d5dfafe62ef4eafd274f295a0fca6ed3c75) - GH-126 Correctly select last open file
- [20665b2](http://github.com/CraterCrash/godot-orchestrator/commit/20665b22a3c9fb8eb716a86bcb85fccf473d082a) - GH-132 Improved graph node rendering
- [16eab41](http://github.com/CraterCrash/godot-orchestrator/commit/16eab414ec4cf0e2f09530e8c2fc5cd2311b55e0) - GH-130 Improve filtering of All Actions dialog
- [8afc997](http://github.com/CraterCrash/godot-orchestrator/commit/8afc997f15c7b5154e4cf2f417fad0bdaf59a256) - GH-126 Render file list like Script tab file list
- [420505a](http://github.com/CraterCrash/godot-orchestrator/commit/420505a3e52c1f1cb2ebee3b4192d1e7f52ab7a7) - GH-111 Correctly handle return type as Variant
- [e3def5b](http://github.com/CraterCrash/godot-orchestrator/commit/e3def5b21a3bbe14c13ee45f48779f44c7ff8a4f) - [ci] Add build concurrency
- [7d67fa4](http://github.com/CraterCrash/godot-orchestrator/commit/7d67fa48b35b6c5aeec518e2fb776d83b39c1728) - [ci] Temporarily disable rsync
- [6c775b3](http://github.com/CraterCrash/godot-orchestrator/commit/6c775b34d9d8054fd573ebcbcd21cfdd6c88f748) - [ci] Remove rsync delete
- [b5fc6f8](http://github.com/CraterCrash/godot-orchestrator/commit/b5fc6f8af0826f3ca9be369318b3a8686ba4a2e2) - [ci] Temporarily rsync docs
- [5710786](http://github.com/CraterCrash/godot-orchestrator/commit/5710786b48ade7225a299716710f736232e5a829) - [ci] Fix documentation workflow condition
- [92332e8](http://github.com/CraterCrash/godot-orchestrator/commit/92332e886c3a1c7b37e984d80788d2e1d0f91a80) - [ci] New Documentation Workflow Builder
- [7c398b2](http://github.com/CraterCrash/godot-orchestrator/commit/7c398b2ad0c6af11712062aff1712102bbd97d1c) - GH-112 Correctly coerce `ComposeFrom` values
- [e91d54f](http://github.com/CraterCrash/godot-orchestrator/commit/e91d54f1053a22d0fcba67bf685ad421cbe74468) - GH-116 Correctly update exported default values on nodes
- [665ec25](http://github.com/CraterCrash/godot-orchestrator/commit/665ec25d4f86f945ba4c93b9fbe1e795b6923608) - GH-114 Apply editor display scale to toolbar
- [f705d1e](http://github.com/CraterCrash/godot-orchestrator/commit/f705d1eea449530db468f26e6e902df5df836c49) - [CMake] Add MacOS universal binary (x86_64,arm64) support

## [2.0.dev3](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.0.dev3) - 2024-02-25

- [862aa9d](http://github.com/CraterCrash/godot-orchestrator/commit/862aa9d310da3a4e900f5ec126a14c266bace010) - GH-84 Support favorites in "All actions" menu
- [ceee2e8](http://github.com/CraterCrash/godot-orchestrator/commit/ceee2e80f521196379aa39509b62ab662d31fafe) - [ci] Upgrade metcalfc/changelog-generator from 4.3.0 to 4.3.1
- [cfdf3e8](http://github.com/CraterCrash/godot-orchestrator/commit/cfdf3e8a3e614430da5c636fb03085dbecd5afd3) - [ci] Remove show files and retest upload
- [843b800](http://github.com/CraterCrash/godot-orchestrator/commit/843b800dc901272b492d83fa7e81bc5323d5addc) - [ci] Show files after download
- [0a381e2](http://github.com/CraterCrash/godot-orchestrator/commit/0a381e268a139661a1ca8ffdcc4e8a69090c6c6a) - [ci] Remove geekyeggo/delete-artifact
- [ecc87c4](http://github.com/CraterCrash/godot-orchestrator/commit/ecc87c408954dcebd503326f5531f79c007d08e0) - [ci] Upgrade actions/upload-artifact from v3 to v4
- [8e6b5a0](http://github.com/CraterCrash/godot-orchestrator/commit/8e6b5a0e4478ec3e5c2bd28d8f1421305ca90685) - [ci] Replace jwlawson/actions-setup-cmake@v1.14 with get-cmake@v3.28.0
- [93d6f64](http://github.com/CraterCrash/godot-orchestrator/commit/93d6f6439d977cd6d33b37db2c6b63a62268af8d) - [ci] Upgrade ilammy/msvc-dev-cmd from 1.12.1 to 1.13.0
- [9bfa8f5](http://github.com/CraterCrash/godot-orchestrator/commit/9bfa8f50dff93517f906f34ed19edf1ffc037c8f) - [ci] Upgrade actions/setup-python from 4 to 5
- [d4bc2ea](http://github.com/CraterCrash/godot-orchestrator/commit/d4bc2eabc2429fee375b92366d762bcfbcb0d239) - GH-74 Add action menu database cache
- [b14f433](http://github.com/CraterCrash/godot-orchestrator/commit/b14f4332ec8f7aa8c929373538b4dd312ea8e69e) - [perf]: minimize function call result variant copies
- [f91ac2f](http://github.com/CraterCrash/godot-orchestrator/commit/f91ac2ff02248aba826d86cf6fd8aaa7beba305c) - [perf]: optimize variant usage in operator node
- [ed2da2f](http://github.com/CraterCrash/godot-orchestrator/commit/ed2da2f31a4748781c9ba1757a201626200c09d3) - [perf]: reduce hash-map lookup by caching function node instance pair
- [f2fdf5f](http://github.com/CraterCrash/godot-orchestrator/commit/f2fdf5f7290f781c4d164b04fca8b4447eb34c24) - [perf]: optimize ComposeFrom node behavior
- [7d4f042](http://github.com/CraterCrash/godot-orchestrator/commit/7d4f0427783e366b4455913ee30ec67840e3b2e5) - [perf]: rework execution context and stack
- [1a83ee8](http://github.com/CraterCrash/godot-orchestrator/commit/1a83ee83d5796a96438105bb9f60e989b2cafcc6) - [perf]: use cached script node identifier
- [f14f82e](http://github.com/CraterCrash/godot-orchestrator/commit/f14f82e1ee8c1933b2874b14f8d04fcc75550017) - [perf]: allow multiple return nodes in function graphs
- [96c2e5b](http://github.com/CraterCrash/godot-orchestrator/commit/96c2e5be9e3225000599e5f9ba7cc7c3810a7c4d) - [perf]: call function node improvements
- [c60bfa4](http://github.com/CraterCrash/godot-orchestrator/commit/c60bfa464b4c524ef8447cd5b2b957ae3303fb9d) - [perf]: clear_error is now no-op if no error is set
- [679139d](http://github.com/CraterCrash/godot-orchestrator/commit/679139d512eb6e6ecaa17ad9acd033491b4e83bf) - [perf]: allocate empty working memory register statically
- [2f52346](http://github.com/CraterCrash/godot-orchestrator/commit/2f52346eeac9048c3efb26ae276d31334ae8d07f) - [perf]: scope ERR_FAIL_xxx macros to DEBUG builds
- [2eda2af](http://github.com/CraterCrash/godot-orchestrator/commit/2eda2af72c85d73df3d1557e11685259bd5073fe) - [perf]: use std::vector<int> and std::lower_bound
- [de2a0a8](http://github.com/CraterCrash/godot-orchestrator/commit/de2a0a8aa147bb32630c6e44fa7d10f5105b8595) - [perf]: cache settings/runtime/max_call_stack
- [8d875ef](http://github.com/CraterCrash/godot-orchestrator/commit/8d875ef049f59042cd4ea25601440d9720f53a7e) - GH-92 Add short-cut keybindings for Variable drag-n-drop
- [e34c057](http://github.com/CraterCrash/godot-orchestrator/commit/e34c0575e7060bb9f2f55d3a5e9325b596d23414) - GH-89 On rename, notify user if new name already exists
- [fe8a393](http://github.com/CraterCrash/godot-orchestrator/commit/fe8a3935e655b2726cb11b0ebe125e18c7e137cf) - GH-89 Correctly resolve new variable names on insert
- [70c0233](http://github.com/CraterCrash/godot-orchestrator/commit/70c023353490014f83db29fdaf21f8b34a2443a5) - GH-82 Fix crash when dragging from node pins
- [2d0c5eb](http://github.com/CraterCrash/godot-orchestrator/commit/2d0c5eb818a215b382806a1495707c4d09dd1d5c) - [ci] Upgrade metcalfc/changelog-generator from 4.2.0 to 4.3.0
- [7defe77](http://github.com/CraterCrash/godot-orchestrator/commit/7defe77aecdb3f14ab15ea06768c2e36ec7c61bb) - [ci] Upgrade robinraju/release-downloader from 1.8 to 1.9

## [2.0.dev2](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.0.dev2) - 2024-01-21

- [ce09546](http://github.com/CraterCrash/godot-orchestrator/commit/ce09546d244ceb9fa750cc6220b410ca42824201) - GH-79 Avoid using deprecated `move_to_foreground`
- [882e7f2](http://github.com/CraterCrash/godot-orchestrator/commit/882e7f2ff0c435400e230c5420da51df93edcf60) - Use Editor theme in About Dialog
- [c864647](http://github.com/CraterCrash/godot-orchestrator/commit/c864647c50de8447ce72e086fed33031a6df3ab1) - GH-72 Add Await Signal Node Feature
- [dde6d70](http://github.com/CraterCrash/godot-orchestrator/commit/dde6d7095af61fcbe9726cea540bfe490f2db905) - GH-67 Fix random error when key is missing
- [fbe3159](http://github.com/CraterCrash/godot-orchestrator/commit/fbe3159fd3330aee2539705b4f397b9ad466a9b3) - GH-66 Add local variable name and description
- [92b9971](http://github.com/CraterCrash/godot-orchestrator/commit/92b99718a1a8a26383ec049c963c6879e1ddf47a) - GH-67 Restore open orchestrations
- [df28556](http://github.com/CraterCrash/godot-orchestrator/commit/df285569af773443bfc19efe2b5bfe0635882a9f) - GH-68 Fix `max_call_stack` property key
- [14c11c2](http://github.com/CraterCrash/godot-orchestrator/commit/14c11c28d4591e3a44ee5086864ed2eeb05e8765) - GH-52 Fix editor crash due to nullptr
- [f2e3766](http://github.com/CraterCrash/godot-orchestrator/commit/f2e37662f98bcbad8f0e34c6eb81e3cee2011dfc) - Add additional Help-menu options
- [b5b3421](http://github.com/CraterCrash/godot-orchestrator/commit/b5b3421a476fdd121ff6c9d159a9918ce79759e8) - GH-52 Work with scene node properties & methods
- [30b6df6](http://github.com/CraterCrash/godot-orchestrator/commit/30b6df629d0f944c207d4158e31378882d5e6494) - GH-61 Add floating window mode
- [90bea05](http://github.com/CraterCrash/godot-orchestrator/commit/90bea051b5f16f4095aad560175ddd27cf257841) - GH-56 Display status text on empty graphs
- [500b0c1](http://github.com/CraterCrash/godot-orchestrator/commit/500b0c1e0fef79d884ae2de9b29993aa22f2a30a) - GH-54 Remove print statement
- [2e666ce](http://github.com/CraterCrash/godot-orchestrator/commit/2e666ce8f05b3dfcd9b9a44c238c5b50e1169996) - GH-54 Add cut/copy/paste/duplicate support
- [06f80b7](http://github.com/CraterCrash/godot-orchestrator/commit/06f80b7baf6824cd8434349ab53a0084a4f7c719) - Upgrade hugoalh/scan-virus-ghaction to 0.20.1
- [85aaacf](http://github.com/CraterCrash/godot-orchestrator/commit/85aaacf39309bb67708920d35e03d72a9f174ac8) - GH-48 Fix rendering of "Update Available" on dev builds
- [f1572bf](http://github.com/CraterCrash/godot-orchestrator/commit/f1572bff1bee46bc82021e1d628699714c9424f0) - Add GitHub Security Workflow
- [625429e](http://github.com/CraterCrash/godot-orchestrator/commit/625429e26b6b40f1da069e3dad567b83eae2b1de) - Add Zoom percentage graph indicator
- [94c85be](http://github.com/CraterCrash/godot-orchestrator/commit/94c85beb56ef14be843f1704122e2086bf8d6ef9) - Add Goto Node accelerator
- [f614bdc](http://github.com/CraterCrash/godot-orchestrator/commit/f614bdcfbed99a72bc54a1c24c3480ffd25b02b2) - Refactor/Cleanup code

## [2.0.dev1](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v2.0.dev1) - 2024-01-01

- [511348b](http://github.com/CraterCrash/godot-orchestrator/commit/511348bdfd863d480de6bd710d5a132b91ede92c) - GH-43 Fix signal function registration
- [dbd29e4](http://github.com/CraterCrash/godot-orchestrator/commit/dbd29e440a502d7808a59291aff598e6d8d3584c) - GH-43 Use default disconnect behavior
- [39d9d58](http://github.com/CraterCrash/godot-orchestrator/commit/39d9d584b48c2135690c4afee44343b69f7677b8) - GH-43 Fix function entry deletion bug
- [dca1b27](http://github.com/CraterCrash/godot-orchestrator/commit/dca1b27a7a9a8cf40dfe2db819cc52352563614a) - GH-43 Update documentation for 2.0
- [927483c](http://github.com/CraterCrash/godot-orchestrator/commit/927483c1f65dc47dfa154c1dcb084dd795e1d24c) - GH-43 Build adjustments
- [741e4eb](http://github.com/CraterCrash/godot-orchestrator/commit/741e4eb7eacad789a511a91a01b49292dfd02e0e) - GH-43 Fix Cleanup build errors/warnings
- [ffbdb3c](http://github.com/CraterCrash/godot-orchestrator/commit/ffbdb3cd1dcf3d4e11475260bdc3a97a9bb28818) - GH-43 Move to ubuntu-latest runners
- [a9f6061](http://github.com/CraterCrash/godot-orchestrator/commit/a9f6061b2fd9378abb62f29a4a9600ffecee675f) - GH-43 Build adjustments
- [d3e53a4](http://github.com/CraterCrash/godot-orchestrator/commit/d3e53a4380812a6f14afcb6b48e5686d70127e70) - GH-43 More cmake fixes
- [4d8aeaa](http://github.com/CraterCrash/godot-orchestrator/commit/4d8aeaa8f820919068e863e68973ce6160798c43) - GH-43 Fix commit hash check
- [760000d](http://github.com/CraterCrash/godot-orchestrator/commit/760000d05b41c34d428093e44bf7a3fe082bbb91) - GH-43 Disable clang-format on build for now
- [ebeff77](http://github.com/CraterCrash/godot-orchestrator/commit/ebeff77c2477c8be733e547f75c2fd6db35a0bcf) - GH-43 Fix clang-format
- [2842457](http://github.com/CraterCrash/godot-orchestrator/commit/284245762d0839156afd0244fb5b8c4794a254ac) - GH-43 Fix incorrect include
- [308a13c](http://github.com/CraterCrash/godot-orchestrator/commit/308a13c59869a18782a4bd4c690348d37c9b8029) - GH-43 Remove unintended inclusion
- [da563dc](http://github.com/CraterCrash/godot-orchestrator/commit/da563dc027d65fc494489d86c08a065bb6f17e75) - GH-43 Use C++20 rather than C++23
- [e8539f5](http://github.com/CraterCrash/godot-orchestrator/commit/e8539f52f26a9dd4e746b8295983f67297cf6892) - GH-43 Port to GDExtension (Godot 4.2+)
- [bf08967](http://github.com/CraterCrash/godot-orchestrator/commit/bf089671a1f9ff5b28a56f2d2e519311726a0286) - GH-45 Only export `/addons/**`

## [1.1.0](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v1.1.0) - 2023-12-08

- [e98d83a](http://github.com/CraterCrash/godot-orchestrator/commit/e98d83ab4d6c90e5fa3cd2af49fbd06f7e8d39b0) - GH-22 Initial supoport for Godot 4.2 (4.2.dev5)
- [bb16856](http://github.com/CraterCrash/godot-orchestrator/commit/bb1685623f985e5b3411cf2028d5bcf86fd4b45d) - [ci] Upgrade metcalfc/changelog-generator from 4.1.0 to 4.2.0
- [867eb58](http://github.com/CraterCrash/godot-orchestrator/commit/867eb58e47d7dbc6c96089d6f378ef926524a876) - Update READMEs
- [90b73b6](http://github.com/CraterCrash/godot-orchestrator/commit/90b73b6b5150f8feb4068a2e8d3a1457a6d71000) - Code improvements
- [4b8c265](http://github.com/CraterCrash/godot-orchestrator/commit/4b8c2656d23b968ce67f6954d842683357b96490) - Add context-menu to Orchestration files list
- [ca1ae29](http://github.com/CraterCrash/godot-orchestrator/commit/ca1ae291e7e6ad570987ff0a4c016bbcf1a4fd41) - GH-24 Introduce an `OrchestrationPlayer` scene node
- [bd20168](http://github.com/CraterCrash/godot-orchestrator/commit/bd201684a3c5b8307e1bccdb07ff0d25a3e8d599) - Update .gitignore
- [292e73c](http://github.com/CraterCrash/godot-orchestrator/commit/292e73c03b62a3e039fb9e3af0b05224536ad842) - GH-33 Add node icons to node list and drag visual
- [266e828](http://github.com/CraterCrash/godot-orchestrator/commit/266e8282982ce273b5fbae5c2fcef0cb519b2e35) - GH-31 Add auto-update feature
- [33684d3](http://github.com/CraterCrash/godot-orchestrator/commit/33684d3ea82079f6e109f11ab4f7dcfd249308cc) - GH-29 Add configurable options to Project Settings UI
- [e20bfc7](http://github.com/CraterCrash/godot-orchestrator/commit/e20bfc74ac86c57481c3fd4a806c6dd181f6d473) - Update README.md
- [4b33aa6](http://github.com/CraterCrash/godot-orchestrator/commit/4b33aa6aa9173d791cfe3d050ef58c9a061bcbf3) - Update README.md
- [7001d13](http://github.com/CraterCrash/godot-orchestrator/commit/7001d1372606c41e9172c9b9c89088a14a05444d) - GH-26 Fix when removing unconnected show message choices
- [26be9f4](http://github.com/CraterCrash/godot-orchestrator/commit/26be9f433d4173a2c57a25df5480ea3f6bea46fc) - GH-21 Show character name field by default
- [277fbd8](http://github.com/CraterCrash/godot-orchestrator/commit/277fbd8a9ea5700ec9af148464bc1da2088cf825) - GH-21 Use correct speaker name for show message node
- [0b6cf15](http://github.com/CraterCrash/godot-orchestrator/commit/0b6cf15f2aba769d4fd1e1cc6ef753721588b3a1) - Add feature request/enhancement issues option
- [027d13b](http://github.com/CraterCrash/godot-orchestrator/commit/027d13bdf77aa926ce3c143299eeda0676b1b4ae) - Update config.yml

## [1.0.5](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v1.0.5) - 2023-09-17

- [0cb4a85](http://github.com/CraterCrash/godot-orchestrator/commit/0cb4a854542f3afffec2b35b241d0ab06a6b677e) - Document wait-for-input-action (#14)
- [0ee6ec3](http://github.com/CraterCrash/godot-orchestrator/commit/0ee6ec38a641b00a487a7e217620a84c0f079ff3) - wait between each action when chaining action multiple times
- [db147c4](http://github.com/CraterCrash/godot-orchestrator/commit/db147c407805a1914b37d7faf33e230da4d4fc09) - adjusted node ui size
- [eacbb2e](http://github.com/CraterCrash/godot-orchestrator/commit/eacbb2ee16a889ccab65373b35af28d7b4d88ced) - renamed shadow variable
- [750e959](http://github.com/CraterCrash/godot-orchestrator/commit/750e9598580f5b2733141a24f987a9be401b34ae) - added wait for action node
- [3fb7b41](http://github.com/CraterCrash/godot-orchestrator/commit/3fb7b41c8282c3eff27d36d7ab4bc44593bba496) - Fix more load order dependency failures
- [402152d](http://github.com/CraterCrash/godot-orchestrator/commit/402152de0606cc6fad34223c849603c40d74183c) - ShowMessage: ESC/SPACE now auto-completes text
- [f585b99](http://github.com/CraterCrash/godot-orchestrator/commit/f585b99a9e5a69d10d263394555dc9b51d2e0115) - Add handles to AUTHORS.txt
- [8275543](http://github.com/CraterCrash/godot-orchestrator/commit/827554380df7218b3bb82bf4c2d8f8b641fe1ad6) - Source version from plugin.cfg
- [a162a9b](http://github.com/CraterCrash/godot-orchestrator/commit/a162a9b156bb6bba82528b5ca365e88e4e2a68d6) - Add GitHub actions & dependabot


## [1.0.4](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v1.0.4) - 2023-09-16

- [4f73e27](https://github.com/CraterCrash/godot-orchestrator/tree/4f73e27eb97f5d20dc89a5685140db846a98a085) - Prepare 1.0.4 release (update version)


## [1.0.3](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v1.0.3) - 2023-09-16

- [4c14f2f](https://github.com/CraterCrash/godot-orchestrator/tree/4c14f2ff1e4c945c3e05053e7b4fd7d2b7e0be08) - Update open file references when moving files
- [7cfc9aa](https://github.com/CraterCrash/godot-orchestrator/tree/7cfc9aa06b1eaad12fd4903517c2f2999cf4c505) - Cleanup editor runtime output
- [fba16d4](https://github.com/CraterCrash/godot-orchestrator/tree/fba16d4289f631369469673e87f162e0b42e22b0) - Fix running orchestrations in editor
- [6c30b52](https://github.com/CraterCrash/godot-orchestrator/tree/6c30b5297ea33742ab13d0a832b8c6554ece64b2) - Allow multiple end nodes
- [b18a349](https://github.com/CraterCrash/godot-orchestrator/tree/b18a34924fc749df2fb2d3fbff82125659d93a1a) - Rename README to README.md
- [d6d7f96](https://github.com/CraterCrash/godot-orchestrator/tree/d6d7f96526b69815207a556bfb40037625768ad7) - Duplicate README in addon folder
- [f6e9fd9](https://github.com/CraterCrash/godot-orchestrator/tree/f6e9fd9a2a88f83b737aa956b034470f8708d019) - Updated README.md


## [1.0.2](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v1.0.2) - 2023-09-10

- [a82afa7](https://github.com/CraterCrash/godot-orchestrator/tree/a82afa7853eff1eb8250b6e3a1d5a1e401787a2b) - Save orchestrations on EditorPlugin apply_changes callback
- [33a1392](https://github.com/CraterCrash/godot-orchestrator/tree/33e1392f63ad8e288ddf6bc990a6c6b22a2d33bc) - Fix plug-in load order, removing const preloads
- [3b54509](https://github.com/CraterCrash/godot-orchestrator/tree/3b54509912d13ac0cc3daf37ba2d202f0cca5dd3) - Load resource scripts safely to avoid runtime errors


## [1.0.1](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v1.0.1) - 2023-09-08

- Removed unused logos.


## [1.0.0](https://github.com/CraterCrash/godot-orchestrator/releases/tag/v1.0.0) - 2023-08-13

- Initial release