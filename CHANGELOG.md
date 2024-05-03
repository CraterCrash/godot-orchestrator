# CHANGELOG

## [2.0.1.stable](https://github.com/Vahera/godot-orchestrator/releases/tag/v2.0.1.stable) - 2024-05-03

- [9291ed1](http://github.com/Vahera/godot-orchestrator/commit/9291ed177e991ba31fc46df9f5daad3e97ec6d18) - GH-284 Add support for static functions
- [a3ff06a](http://github.com/Vahera/godot-orchestrator/commit/a3ff06a5af085a6957c1c71405035f05023333b2) - GH-321 Persist `For Each` node's "with break" status
- [996261a](http://github.com/Vahera/godot-orchestrator/commit/996261a4cc8014b9d284efe8b8b3733fed5288aa) - GH-323 Fix persisting enum default value choices
- [0add2f9](http://github.com/Vahera/godot-orchestrator/commit/0add2f9e5d21491493b006e2b252cdd3acbeeccf) - GH-315 Have `For Each` set Element as `Any`
- [2a4f031](http://github.com/Vahera/godot-orchestrator/commit/2a4f031038621bad9a5240079c14e89bad5195ba) - GH-318 Add `Dictionary Set` node
- [9229c22](http://github.com/Vahera/godot-orchestrator/commit/9229c228dcf770a650c377639c5fb27cb79adee2) - GH-313 Add Godot version compile-time constant
- [f140748](http://github.com/Vahera/godot-orchestrator/commit/f1407482e991584a403f056aa1c58f8c8308dda8) - GH-304 Track recently accessed orchestrations
- [4941e51](http://github.com/Vahera/godot-orchestrator/commit/4941e51187e3c7e2eb8b084db61b13fe2a4f8027) - GH-310 Support file/folder removed and files moved signals
- [157a8ca](http://github.com/Vahera/godot-orchestrator/commit/157a8ca22f9a682bd0728a030eaab520ddfec68f) - GH-306 Refactor `MethodUtils`
- [d9b97d2](http://github.com/Vahera/godot-orchestrator/commit/d9b97d2f2eba011cbf13beb683d15958c1e1ea11) - GH-306 Fix functions to return Any types
- [75e8bfe](http://github.com/Vahera/godot-orchestrator/commit/75e8bfef988e7afdc4134ab8de26c9e72ffe0ff4) - GH-287 Always call "Init" event if its wired
- [e0c27f4](http://github.com/Vahera/godot-orchestrator/commit/e0c27f405e4e588736c95fd4161a2f0d1cbb4df3) - GH-283 Support method chaining
- [2b554f5](http://github.com/Vahera/godot-orchestrator/commit/2b554f52528112c270dbbd1ba935ae8bd7a87fe0) - GH-289 Fix built-in methods using PROPERTY_USAGE_NIL_IS_VARIANT
- [59eb2d5](http://github.com/Vahera/godot-orchestrator/commit/59eb2d555da2c85f850a9439e4bcf317d0e0eca9) - GH-282 Explicitly filter `_get` and `_set` methods
- [abcf277](http://github.com/Vahera/godot-orchestrator/commit/abcf277fde8a8a26e524e70616eb64278b508f4d) - GH-297 Preserve setting order when setting is modified
- [0e27325](http://github.com/Vahera/godot-orchestrator/commit/0e2732551f8cad543e2f698072d0e713f30525c9) - Fix comparison signed/unsigned mismatch
- [465fd11](http://github.com/Vahera/godot-orchestrator/commit/465fd119421399543dacf464b0ca45ed4a1b17bf) - GH-272 Remove obsolete documentation
- [1279406](http://github.com/Vahera/godot-orchestrator/commit/1279406b1826fbeccf5b9ef182709e8a0c8335a5) - Rework editor icon lookups
- [2c21080](http://github.com/Vahera/godot-orchestrator/commit/2c21080ee61e467ef71914987d41ab52e7199055) - GH-206 Correctly generates action items for instantiated scenes
- [6afd3fa](http://github.com/Vahera/godot-orchestrator/commit/6afd3fad145e50a5f7c9856c457f63aff84433c5) - GH-205 Dim wires when using `highlight_selected_connections`
- [ea83482](http://github.com/Vahera/godot-orchestrator/commit/ea83482d1341f545f726641f7ce806fe6ecf32aa) - GH-248 Construct `Callable` with correct target reference
- [8f0cc53](http://github.com/Vahera/godot-orchestrator/commit/8f0cc53db1398efdf560da78828d511a50c13c39) - GH-273 Force `PrintStringUI` to ignore mouse events
- [599e925](http://github.com/Vahera/godot-orchestrator/commit/599e9255fe611b781522831393048ff58cd4b60b) - GH-254 Fix emitting non-Orchestrator based signals
- [9f5e025](http://github.com/Vahera/godot-orchestrator/commit/9f5e0252a7addfacadff6985408255e3e6050c6b) - GH-238 Rework/Improve Bitfield pin value rendering
- [cd59f49](http://github.com/Vahera/godot-orchestrator/commit/cd59f49034b928946d04beeec11148c3abf1ecee) - GH-265 Support Object `new()` and `free()` nodes
- [ccecadb](http://github.com/Vahera/godot-orchestrator/commit/ccecadb65dc35db6cdec6d7d29837d660366131b) - GH-266 Correctly resolve property class actions
- [cc6ee3f](http://github.com/Vahera/godot-orchestrator/commit/cc6ee3ff845b730d1ed33f9b9f914c4d26bc00b3) - GH-253 Include registered autoloads in all actions menu
- [ddde63c](http://github.com/Vahera/godot-orchestrator/commit/ddde63c8cfc64baa2817dac421635faf66c9cdf0) - GH-268 Do not serialize OScriptNode flags
- [a8aed0b](http://github.com/Vahera/godot-orchestrator/commit/a8aed0b18b16e263f5dfc4eb8d5b0f96e2e85a2b) - Don't use bitwise `&` operator, use correct operator
- [9a69d91](http://github.com/Vahera/godot-orchestrator/commit/9a69d91ab5ee7b634126894bb2e547a4b27575cb) - Don't make unnecessary copies of OScriptGraph
- [51b3b71](http://github.com/Vahera/godot-orchestrator/commit/51b3b7155f7a26b44ff3b61a61805e4bb8e93708) - [ci] Android artifacts use 'lib' prefix
- [081c653](http://github.com/Vahera/godot-orchestrator/commit/081c6536b2a62789489947174a417a18e16a59fe) - [ci] Disable debug builds
- [0d6d8c1](http://github.com/Vahera/godot-orchestrator/commit/0d6d8c11ee96b601ebabfba7cfbc258ba5c66cef) - [ci] Add android build
- [051dc79](http://github.com/Vahera/godot-orchestrator/commit/051dc79ba66c78c290150b9c9c45d8c962195442) - GH-245 Force `ExtensionDB` generator to use LF line endings
- [fca96a1](http://github.com/Vahera/godot-orchestrator/commit/fca96a1fca66d0bc2b7e1bacce129867d74a04a4) - GH-239 Provide `ExtensionDB::is_class_enum_bitfield` look-up
- [c462a4d](http://github.com/Vahera/godot-orchestrator/commit/c462a4d12a4864e596a188a9ef9b2770e9ca6dc1) - GH-237 Render @GlobalScope and class-scope bitfields uniformly
- [d8731c1](http://github.com/Vahera/godot-orchestrator/commit/d8731c1578c394b3d17b179830ea34d970b4d677) - GH-240 Standardize Method return value check
- [adf8b73](http://github.com/Vahera/godot-orchestrator/commit/adf8b73e07d698060ad7113ff6d25235d91b3ec9) - GH-217 Guard against nullptr
- [d746c9b](http://github.com/Vahera/godot-orchestrator/commit/d746c9bf89b407ab2c80146c344f54bcbbadde5a) - GH-217 Add signal connection indicator for user-defined functions
- [bae4ad1](http://github.com/Vahera/godot-orchestrator/commit/bae4ad132bf1504a9f3f16ea39ba52e204a34ac1) - GH-232 Remove superfluous warnings saving callable/signal types
- [efeef90](http://github.com/Vahera/godot-orchestrator/commit/efeef9084e22939496c746ce7c6016f92c6591d8) - GH-230 Reconstruct existing node on sync request
- [5a2eb40](http://github.com/Vahera/godot-orchestrator/commit/5a2eb40ab179b815ea068c7fd9ee4417ff0b7016) - GH-222 Allow some constant/singleton names be searchable
- [c876ac2](http://github.com/Vahera/godot-orchestrator/commit/c876ac2b8203368229a1cda0c4e873ce619c5c68) - GH-227 Correctly encode Godot vararg built-in functions
- [138b3f2](http://github.com/Vahera/godot-orchestrator/commit/138b3f29c403a596d4bb3f406cebc0178dabba45) - GH-207 Change orchestration view based on active root scene node
- [9f42108](http://github.com/Vahera/godot-orchestrator/commit/9f4210813fb6739e29800dd1159496717d6f5e0e) - GH-204 Spawn override function at center of view
- [664cc45](http://github.com/Vahera/godot-orchestrator/commit/664cc4524ef73c4188c10df1b100d0fc4a7f9c7e) - GH-215 Initially set function arguments to default values

## [2.0.stable](https://github.com/Vahera/godot-orchestrator/releases/tag/v2.0.stable) - 2024-03-30

- [280aadc](http://github.com/Vahera/godot-orchestrator/commit/280aadcb5c592722e52718af4bb247c249dc5a57) - GH-202 Fix disconnect error on `tab_changed` signal
- [30d6299](http://github.com/Vahera/godot-orchestrator/commit/30d6299a21786024e9b1669ff826f2b0b789a3fe) - GH-208 Fix patch/build version check logic
- [5038580](http://github.com/Vahera/godot-orchestrator/commit/503858047c6c90b28a57351aed7f6e080de96602) - GH-208 Add auto-update functionality
- [ecbdebf](http://github.com/Vahera/godot-orchestrator/commit/ecbdebf2fd3e96cad724c16788d47ca7e1f1fba1) - [ci] Upgrade tj-actions/changed-files from v43.0.1 to v44.0.0

## [2.0.rc2](https://github.com/Vahera/godot-orchestrator/releases/tag/v2.0.rc2) - 2024-03-24

- [65f82f2](http://github.com/Vahera/godot-orchestrator/commit/65f82f2229f09fd04b35960c1954c3ba44805881) - Rework about dialog
- [3b17834](http://github.com/Vahera/godot-orchestrator/commit/3b1783489887777c51b1291c51dbbb2dc7d48243) - GH-176 Move dialog center prior to popup
- [10034e3](http://github.com/Vahera/godot-orchestrator/commit/10034e3ec8df8ef76179298752d07ff79932ff3c) - GH-193 Allow highlighting selected nodes/connections
- [1e4ae2e](http://github.com/Vahera/godot-orchestrator/commit/1e4ae2e6b6ab56cbef3432878bfd5fd9e2f3a0e8) - GH-183 Signal function connection indicator/dialog
- [34f85f8](http://github.com/Vahera/godot-orchestrator/commit/34f85f877dec17f4526ac898b2086f72f56322a3) - GH-177 Fix type propagation for Preload and Instantiate Scene
- [e769057](http://github.com/Vahera/godot-orchestrator/commit/e7690574fa98026435ab8d35369e62a8734ecaf7) - GH-194 Reduce title text in call function nodes
- [57e13ce](http://github.com/Vahera/godot-orchestrator/commit/57e13ce430ddab059df0fa4757309acc53ccc501) - GH-187 Add BitField input/default value support
- [3599beb](http://github.com/Vahera/godot-orchestrator/commit/3599bebde3aaab834fca4c71c58c0332e7bf5896) - GH-180 Support variadic function arguments
- [5e36291](http://github.com/Vahera/godot-orchestrator/commit/5e3629180135fdfffea93d0b111877ae172ff573) - GH-190 Do not cache instantiated scene node
- [fdec82f](http://github.com/Vahera/godot-orchestrator/commit/fdec82f958e94c2aa82010d0eace46f1f3e247dd) - GH-179 Remove Function Call `(self)` indicator from non-target pins
- [1590194](http://github.com/Vahera/godot-orchestrator/commit/1590194efb24248638536029b65386742ce73742) - GH-179 Function Call `Target` pin no longer autowired
- [b19db62](http://github.com/Vahera/godot-orchestrator/commit/b19db62f31e78495f47ebae0af2f4ca4d539ea81) - GH-182 The `ENTER` key releases node input focus
- [4e146cb](http://github.com/Vahera/godot-orchestrator/commit/4e146cba5eeedf8a9fb3a5abea1c3c8191c71a0e) - GH-176 Position all actions menu based on `center_on_mouse`
- [2b95ddd](http://github.com/Vahera/godot-orchestrator/commit/2b95ddd35423082b89b9ea621deaa516acbf0e2d) - [ci] Upgrade tj-actions/changed-files from v43.0.0 to v43.0.1
- [d84191d](http://github.com/Vahera/godot-orchestrator/commit/d84191dbc4282f7cb33ef19faf9cbb7b78134179) - GH-173 Align styles with non-comment nodes
- [6cd0b66](http://github.com/Vahera/godot-orchestrator/commit/6cd0b66aea13d6d8d9d0cca37bc5e55bd5f96f55) - GH-173 Sync comment background color on inspector change
- [df24b23](http://github.com/Vahera/godot-orchestrator/commit/df24b23d6e6a877a41ba07b5f15bea12d9ca0cfb) - Update README.md
- [551ed1f](http://github.com/Vahera/godot-orchestrator/commit/551ed1f738fe64c8c11d1a4ae37afe648198fa51) - GH-98 Distribute Orchestrator icon .import files
- [df6691f](http://github.com/Vahera/godot-orchestrator/commit/df6691fff827a8df9f69a16570ac1b25b4fa3eb5) - GH-152 Do not change node icons when script attached
- [269e8e9](http://github.com/Vahera/godot-orchestrator/commit/269e8e9f2079226bf9c33634ee1dd75999f694b5) - GH-167 Use `ubuntu-20.04` for GLIBC 2.31
- [161976e](http://github.com/Vahera/godot-orchestrator/commit/161976e26078cf28a0da6add0f76063110ee85c4) - [ci] Upgrade tj-actions/changed-files from v42.1.0 to v43.0.0
- [de49a48](http://github.com/Vahera/godot-orchestrator/commit/de49a480e4512b2095a0a39d4b2b31928b105c80) - GH-164 Add Local Variable types Object & Any
- [38e5b43](http://github.com/Vahera/godot-orchestrator/commit/38e5b43df3614a3b49d4d2436c568b624cc949c4) - GH-158 Correctly escape modulus title text
- [7d77cff](http://github.com/Vahera/godot-orchestrator/commit/7d77cffe6da374574acd20e9c619a7ec9fd04eaa) - GH-155 Allow toggling component panel visibility
- [6113a9c](http://github.com/Vahera/godot-orchestrator/commit/6113a9c4b84a79497effa07d74173b92df5bf7c4) - GH-159 Support all Packed-Array types
- [63011e1](http://github.com/Vahera/godot-orchestrator/commit/63011e1d397232321bc948298659641c75b5cea1) - [ci] Upgrade tj-actions/changed-files from v42.0.7 to v42.1.0
- [7c6731e](http://github.com/Vahera/godot-orchestrator/commit/7c6731e72793b774d4d7b1fe9c6b796255100f0d) - Update documentation workflow

## [2.0.rc1](https://github.com/Vahera/godot-orchestrator/releases/tag/v2.0.rc1) - 2024-03-10

- [468efc6](http://github.com/Vahera/godot-orchestrator/commit/468efc63bf36ac24848a7a709fcbf40ef87cd795) - [docs] Update tutorial
- [5522fd1](http://github.com/Vahera/godot-orchestrator/commit/5522fd11df1dc027fc0f8b0430133d4285976fe9) - [docs] Update docs images with new styles
- [7ed918c](http://github.com/Vahera/godot-orchestrator/commit/7ed918c8620c7720466df92699c49b4e22d26887) - [docs] Update nodes
- [2b5ba1a](http://github.com/Vahera/godot-orchestrator/commit/2b5ba1a90d9c98b6d0875bdecc1ac93d6396309e) - GH-145 Use `snprintf` instead
- [cbd89b2](http://github.com/Vahera/godot-orchestrator/commit/cbd89b22f5bf149c72070e051a614f2ad6969db4) - GH-145 Use arch_linux compatible time formatting
- [ebb0b0b](http://github.com/Vahera/godot-orchestrator/commit/ebb0b0bf3b24a31f002aeac42140903ed4202e10) - [ci] Upgrade tj-actions/changed-files from v42.0.6 v42.0.7
- [2a64fe4](http://github.com/Vahera/godot-orchestrator/commit/2a64fe44c2a22b815d5484ef571c4adcc1dc8dee) - GH-136 Add build validation for autoload node
- [696f89a](http://github.com/Vahera/godot-orchestrator/commit/696f89a1f729438889bb0891e2eea792ee97db6d) - GH-136 Add support for accessing Godot autoloads
- [dfb8ece](http://github.com/Vahera/godot-orchestrator/commit/dfb8eceec8c5c39c5830f6ec78697e84bdb4632a) - GH-143 Return null rather than 0-sized property info list
- [290f57e](http://github.com/Vahera/godot-orchestrator/commit/290f57ee5406351591d632d61551d56618e1ebb1) - [ci] Upgrade tj-actions/changed-files from v42.0.5 to v42.0.6
- [573abaa](http://github.com/Vahera/godot-orchestrator/commit/573abaa18b2b122489999ff45e451e79d813e49b) - GH-122 Introduce `Instantiate Scene` node
- [92c7b06](http://github.com/Vahera/godot-orchestrator/commit/92c7b06eb62a032fe968c5fe0529a2eb7e5842a9) - GH-122 Make file pin behavior generic
- [2f84493](http://github.com/Vahera/godot-orchestrator/commit/2f844933ef730124636a81b3d6264466201c584c) - GH-118 Fix equality ambiguity failure
- [3a98226](http://github.com/Vahera/godot-orchestrator/commit/3a982264068d6a61d2da9774ad7655c1930a0a67) - GH-118 Fix NodePath property resolution
- [33a995d](http://github.com/Vahera/godot-orchestrator/commit/33a995ddf8ddcd03525eca56649d8f4c969acd00) - GH-118 Fix GetSceneNode resolution
- [24f9f5a](http://github.com/Vahera/godot-orchestrator/commit/24f9f5af46b2df43027b15c09584eaa0264c8a1d) - GH-121 Dispatch only script user-defined functions
- [19026d5](http://github.com/Vahera/godot-orchestrator/commit/19026d5dfafe62ef4eafd274f295a0fca6ed3c75) - GH-126 Correctly select last open file
- [20665b2](http://github.com/Vahera/godot-orchestrator/commit/20665b22a3c9fb8eb716a86bcb85fccf473d082a) - GH-132 Improved graph node rendering
- [16eab41](http://github.com/Vahera/godot-orchestrator/commit/16eab414ec4cf0e2f09530e8c2fc5cd2311b55e0) - GH-130 Improve filtering of All Actions dialog
- [8afc997](http://github.com/Vahera/godot-orchestrator/commit/8afc997f15c7b5154e4cf2f417fad0bdaf59a256) - GH-126 Render file list like Script tab file list
- [420505a](http://github.com/Vahera/godot-orchestrator/commit/420505a3e52c1f1cb2ebee3b4192d1e7f52ab7a7) - GH-111 Correctly handle return type as Variant
- [e3def5b](http://github.com/Vahera/godot-orchestrator/commit/e3def5b21a3bbe14c13ee45f48779f44c7ff8a4f) - [ci] Add build concurrency
- [7d67fa4](http://github.com/Vahera/godot-orchestrator/commit/7d67fa48b35b6c5aeec518e2fb776d83b39c1728) - [ci] Temporarily disable rsync
- [6c775b3](http://github.com/Vahera/godot-orchestrator/commit/6c775b34d9d8054fd573ebcbcd21cfdd6c88f748) - [ci] Remove rsync delete
- [b5fc6f8](http://github.com/Vahera/godot-orchestrator/commit/b5fc6f8af0826f3ca9be369318b3a8686ba4a2e2) - [ci] Temporarily rsync docs
- [5710786](http://github.com/Vahera/godot-orchestrator/commit/5710786b48ade7225a299716710f736232e5a829) - [ci] Fix documentation workflow condition
- [92332e8](http://github.com/Vahera/godot-orchestrator/commit/92332e886c3a1c7b37e984d80788d2e1d0f91a80) - [ci] New Documentation Workflow Builder
- [7c398b2](http://github.com/Vahera/godot-orchestrator/commit/7c398b2ad0c6af11712062aff1712102bbd97d1c) - GH-112 Correctly coerce `ComposeFrom` values
- [e91d54f](http://github.com/Vahera/godot-orchestrator/commit/e91d54f1053a22d0fcba67bf685ad421cbe74468) - GH-116 Correctly update exported default values on nodes
- [665ec25](http://github.com/Vahera/godot-orchestrator/commit/665ec25d4f86f945ba4c93b9fbe1e795b6923608) - GH-114 Apply editor display scale to toolbar
- [f705d1e](http://github.com/Vahera/godot-orchestrator/commit/f705d1eea449530db468f26e6e902df5df836c49) - [CMake] Add MacOS universal binary (x86_64,arm64) support

## [2.0.dev3](https://github.com/Vahera/godot-orchestrator/releases/tag/v2.0.dev3) - 2024-02-25

- [862aa9d](http://github.com/Vahera/godot-orchestrator/commit/862aa9d310da3a4e900f5ec126a14c266bace010) - GH-84 Support favorites in "All actions" menu
- [ceee2e8](http://github.com/Vahera/godot-orchestrator/commit/ceee2e80f521196379aa39509b62ab662d31fafe) - [ci] Upgrade metcalfc/changelog-generator from 4.3.0 to 4.3.1
- [cfdf3e8](http://github.com/Vahera/godot-orchestrator/commit/cfdf3e8a3e614430da5c636fb03085dbecd5afd3) - [ci] Remove show files and retest upload
- [843b800](http://github.com/Vahera/godot-orchestrator/commit/843b800dc901272b492d83fa7e81bc5323d5addc) - [ci] Show files after download
- [0a381e2](http://github.com/Vahera/godot-orchestrator/commit/0a381e268a139661a1ca8ffdcc4e8a69090c6c6a) - [ci] Remove geekyeggo/delete-artifact
- [ecc87c4](http://github.com/Vahera/godot-orchestrator/commit/ecc87c408954dcebd503326f5531f79c007d08e0) - [ci] Upgrade actions/upload-artifact from v3 to v4
- [8e6b5a0](http://github.com/Vahera/godot-orchestrator/commit/8e6b5a0e4478ec3e5c2bd28d8f1421305ca90685) - [ci] Replace jwlawson/actions-setup-cmake@v1.14 with get-cmake@v3.28.0
- [93d6f64](http://github.com/Vahera/godot-orchestrator/commit/93d6f6439d977cd6d33b37db2c6b63a62268af8d) - [ci] Upgrade ilammy/msvc-dev-cmd from 1.12.1 to 1.13.0
- [9bfa8f5](http://github.com/Vahera/godot-orchestrator/commit/9bfa8f50dff93517f906f34ed19edf1ffc037c8f) - [ci] Upgrade actions/setup-python from 4 to 5
- [d4bc2ea](http://github.com/Vahera/godot-orchestrator/commit/d4bc2eabc2429fee375b92366d762bcfbcb0d239) - GH-74 Add action menu database cache
- [b14f433](http://github.com/Vahera/godot-orchestrator/commit/b14f4332ec8f7aa8c929373538b4dd312ea8e69e) - [perf]: minimize function call result variant copies
- [f91ac2f](http://github.com/Vahera/godot-orchestrator/commit/f91ac2ff02248aba826d86cf6fd8aaa7beba305c) - [perf]: optimize variant usage in operator node
- [ed2da2f](http://github.com/Vahera/godot-orchestrator/commit/ed2da2f31a4748781c9ba1757a201626200c09d3) - [perf]: reduce hash-map lookup by caching function node instance pair
- [f2fdf5f](http://github.com/Vahera/godot-orchestrator/commit/f2fdf5f7290f781c4d164b04fca8b4447eb34c24) - [perf]: optimize ComposeFrom node behavior
- [7d4f042](http://github.com/Vahera/godot-orchestrator/commit/7d4f0427783e366b4455913ee30ec67840e3b2e5) - [perf]: rework execution context and stack
- [1a83ee8](http://github.com/Vahera/godot-orchestrator/commit/1a83ee83d5796a96438105bb9f60e989b2cafcc6) - [perf]: use cached script node identifier
- [f14f82e](http://github.com/Vahera/godot-orchestrator/commit/f14f82e1ee8c1933b2874b14f8d04fcc75550017) - [perf]: allow multiple return nodes in function graphs
- [96c2e5b](http://github.com/Vahera/godot-orchestrator/commit/96c2e5be9e3225000599e5f9ba7cc7c3810a7c4d) - [perf]: call function node improvements
- [c60bfa4](http://github.com/Vahera/godot-orchestrator/commit/c60bfa464b4c524ef8447cd5b2b957ae3303fb9d) - [perf]: clear_error is now no-op if no error is set
- [679139d](http://github.com/Vahera/godot-orchestrator/commit/679139d512eb6e6ecaa17ad9acd033491b4e83bf) - [perf]: allocate empty working memory register statically
- [2f52346](http://github.com/Vahera/godot-orchestrator/commit/2f52346eeac9048c3efb26ae276d31334ae8d07f) - [perf]: scope ERR_FAIL_xxx macros to DEBUG builds
- [2eda2af](http://github.com/Vahera/godot-orchestrator/commit/2eda2af72c85d73df3d1557e11685259bd5073fe) - [perf]: use std::vector<int> and std::lower_bound
- [de2a0a8](http://github.com/Vahera/godot-orchestrator/commit/de2a0a8aa147bb32630c6e44fa7d10f5105b8595) - [perf]: cache settings/runtime/max_call_stack
- [8d875ef](http://github.com/Vahera/godot-orchestrator/commit/8d875ef049f59042cd4ea25601440d9720f53a7e) - GH-92 Add short-cut keybindings for Variable drag-n-drop
- [e34c057](http://github.com/Vahera/godot-orchestrator/commit/e34c0575e7060bb9f2f55d3a5e9325b596d23414) - GH-89 On rename, notify user if new name already exists
- [fe8a393](http://github.com/Vahera/godot-orchestrator/commit/fe8a3935e655b2726cb11b0ebe125e18c7e137cf) - GH-89 Correctly resolve new variable names on insert
- [70c0233](http://github.com/Vahera/godot-orchestrator/commit/70c023353490014f83db29fdaf21f8b34a2443a5) - GH-82 Fix crash when dragging from node pins
- [2d0c5eb](http://github.com/Vahera/godot-orchestrator/commit/2d0c5eb818a215b382806a1495707c4d09dd1d5c) - [ci] Upgrade metcalfc/changelog-generator from 4.2.0 to 4.3.0
- [7defe77](http://github.com/Vahera/godot-orchestrator/commit/7defe77aecdb3f14ab15ea06768c2e36ec7c61bb) - [ci] Upgrade robinraju/release-downloader from 1.8 to 1.9

## [2.0.dev2](https://github.com/Vahera/godot-orchestrator/releases/tag/v2.0.dev2) - 2024-01-21

- [ce09546](http://github.com/Vahera/godot-orchestrator/commit/ce09546d244ceb9fa750cc6220b410ca42824201) - GH-79 Avoid using deprecated `move_to_foreground`
- [882e7f2](http://github.com/Vahera/godot-orchestrator/commit/882e7f2ff0c435400e230c5420da51df93edcf60) - Use Editor theme in About Dialog
- [c864647](http://github.com/Vahera/godot-orchestrator/commit/c864647c50de8447ce72e086fed33031a6df3ab1) - GH-72 Add Await Signal Node Feature
- [dde6d70](http://github.com/Vahera/godot-orchestrator/commit/dde6d7095af61fcbe9726cea540bfe490f2db905) - GH-67 Fix random error when key is missing
- [fbe3159](http://github.com/Vahera/godot-orchestrator/commit/fbe3159fd3330aee2539705b4f397b9ad466a9b3) - GH-66 Add local variable name and description
- [92b9971](http://github.com/Vahera/godot-orchestrator/commit/92b99718a1a8a26383ec049c963c6879e1ddf47a) - GH-67 Restore open orchestrations
- [df28556](http://github.com/Vahera/godot-orchestrator/commit/df285569af773443bfc19efe2b5bfe0635882a9f) - GH-68 Fix `max_call_stack` property key
- [14c11c2](http://github.com/Vahera/godot-orchestrator/commit/14c11c28d4591e3a44ee5086864ed2eeb05e8765) - GH-52 Fix editor crash due to nullptr
- [f2e3766](http://github.com/Vahera/godot-orchestrator/commit/f2e37662f98bcbad8f0e34c6eb81e3cee2011dfc) - Add additional Help-menu options
- [b5b3421](http://github.com/Vahera/godot-orchestrator/commit/b5b3421a476fdd121ff6c9d159a9918ce79759e8) - GH-52 Work with scene node properties & methods
- [30b6df6](http://github.com/Vahera/godot-orchestrator/commit/30b6df629d0f944c207d4158e31378882d5e6494) - GH-61 Add floating window mode
- [90bea05](http://github.com/Vahera/godot-orchestrator/commit/90bea051b5f16f4095aad560175ddd27cf257841) - GH-56 Display status text on empty graphs
- [500b0c1](http://github.com/Vahera/godot-orchestrator/commit/500b0c1e0fef79d884ae2de9b29993aa22f2a30a) - GH-54 Remove print statement
- [2e666ce](http://github.com/Vahera/godot-orchestrator/commit/2e666ce8f05b3dfcd9b9a44c238c5b50e1169996) - GH-54 Add cut/copy/paste/duplicate support
- [06f80b7](http://github.com/Vahera/godot-orchestrator/commit/06f80b7baf6824cd8434349ab53a0084a4f7c719) - Upgrade hugoalh/scan-virus-ghaction to 0.20.1
- [85aaacf](http://github.com/Vahera/godot-orchestrator/commit/85aaacf39309bb67708920d35e03d72a9f174ac8) - GH-48 Fix rendering of "Update Available" on dev builds
- [f1572bf](http://github.com/Vahera/godot-orchestrator/commit/f1572bff1bee46bc82021e1d628699714c9424f0) - Add GitHub Security Workflow
- [625429e](http://github.com/Vahera/godot-orchestrator/commit/625429e26b6b40f1da069e3dad567b83eae2b1de) - Add Zoom percentage graph indicator
- [94c85be](http://github.com/Vahera/godot-orchestrator/commit/94c85beb56ef14be843f1704122e2086bf8d6ef9) - Add Goto Node accelerator
- [f614bdc](http://github.com/Vahera/godot-orchestrator/commit/f614bdcfbed99a72bc54a1c24c3480ffd25b02b2) - Refactor/Cleanup code

## [2.0.dev1](https://github.com/Vahera/godot-orchestrator/releases/tag/v2.0.dev1) - 2024-01-01

- [511348b](http://github.com/Vahera/godot-orchestrator/commit/511348bdfd863d480de6bd710d5a132b91ede92c) - GH-43 Fix signal function registration
- [dbd29e4](http://github.com/Vahera/godot-orchestrator/commit/dbd29e440a502d7808a59291aff598e6d8d3584c) - GH-43 Use default disconnect behavior
- [39d9d58](http://github.com/Vahera/godot-orchestrator/commit/39d9d584b48c2135690c4afee44343b69f7677b8) - GH-43 Fix function entry deletion bug
- [dca1b27](http://github.com/Vahera/godot-orchestrator/commit/dca1b27a7a9a8cf40dfe2db819cc52352563614a) - GH-43 Update documentation for 2.0
- [927483c](http://github.com/Vahera/godot-orchestrator/commit/927483c1f65dc47dfa154c1dcb084dd795e1d24c) - GH-43 Build adjustments
- [741e4eb](http://github.com/Vahera/godot-orchestrator/commit/741e4eb7eacad789a511a91a01b49292dfd02e0e) - GH-43 Fix Cleanup build errors/warnings
- [ffbdb3c](http://github.com/Vahera/godot-orchestrator/commit/ffbdb3cd1dcf3d4e11475260bdc3a97a9bb28818) - GH-43 Move to ubuntu-latest runners
- [a9f6061](http://github.com/Vahera/godot-orchestrator/commit/a9f6061b2fd9378abb62f29a4a9600ffecee675f) - GH-43 Build adjustments
- [d3e53a4](http://github.com/Vahera/godot-orchestrator/commit/d3e53a4380812a6f14afcb6b48e5686d70127e70) - GH-43 More cmake fixes
- [4d8aeaa](http://github.com/Vahera/godot-orchestrator/commit/4d8aeaa8f820919068e863e68973ce6160798c43) - GH-43 Fix commit hash check
- [760000d](http://github.com/Vahera/godot-orchestrator/commit/760000d05b41c34d428093e44bf7a3fe082bbb91) - GH-43 Disable clang-format on build for now
- [ebeff77](http://github.com/Vahera/godot-orchestrator/commit/ebeff77c2477c8be733e547f75c2fd6db35a0bcf) - GH-43 Fix clang-format
- [2842457](http://github.com/Vahera/godot-orchestrator/commit/284245762d0839156afd0244fb5b8c4794a254ac) - GH-43 Fix incorrect include
- [308a13c](http://github.com/Vahera/godot-orchestrator/commit/308a13c59869a18782a4bd4c690348d37c9b8029) - GH-43 Remove unintended inclusion
- [da563dc](http://github.com/Vahera/godot-orchestrator/commit/da563dc027d65fc494489d86c08a065bb6f17e75) - GH-43 Use C++20 rather than C++23
- [e8539f5](http://github.com/Vahera/godot-orchestrator/commit/e8539f52f26a9dd4e746b8295983f67297cf6892) - GH-43 Port to GDExtension (Godot 4.2+)
- [bf08967](http://github.com/Vahera/godot-orchestrator/commit/bf089671a1f9ff5b28a56f2d2e519311726a0286) - GH-45 Only export `/addons/**`

## [1.1.0](https://github.com/Vahera/godot-orchestrator/releases/tag/v1.1.0) - 2023-12-08

- [e98d83a](http://github.com/Vahera/godot-orchestrator/commit/e98d83ab4d6c90e5fa3cd2af49fbd06f7e8d39b0) - GH-22 Initial supoport for Godot 4.2 (4.2.dev5)
- [bb16856](http://github.com/Vahera/godot-orchestrator/commit/bb1685623f985e5b3411cf2028d5bcf86fd4b45d) - [ci] Upgrade metcalfc/changelog-generator from 4.1.0 to 4.2.0
- [867eb58](http://github.com/Vahera/godot-orchestrator/commit/867eb58e47d7dbc6c96089d6f378ef926524a876) - Update READMEs
- [90b73b6](http://github.com/Vahera/godot-orchestrator/commit/90b73b6b5150f8feb4068a2e8d3a1457a6d71000) - Code improvements
- [4b8c265](http://github.com/Vahera/godot-orchestrator/commit/4b8c2656d23b968ce67f6954d842683357b96490) - Add context-menu to Orchestration files list
- [ca1ae29](http://github.com/Vahera/godot-orchestrator/commit/ca1ae291e7e6ad570987ff0a4c016bbcf1a4fd41) - GH-24 Introduce an `OrchestrationPlayer` scene node
- [bd20168](http://github.com/Vahera/godot-orchestrator/commit/bd201684a3c5b8307e1bccdb07ff0d25a3e8d599) - Update .gitignore
- [292e73c](http://github.com/Vahera/godot-orchestrator/commit/292e73c03b62a3e039fb9e3af0b05224536ad842) - GH-33 Add node icons to node list and drag visual
- [266e828](http://github.com/Vahera/godot-orchestrator/commit/266e8282982ce273b5fbae5c2fcef0cb519b2e35) - GH-31 Add auto-update feature
- [33684d3](http://github.com/Vahera/godot-orchestrator/commit/33684d3ea82079f6e109f11ab4f7dcfd249308cc) - GH-29 Add configurable options to Project Settings UI
- [e20bfc7](http://github.com/Vahera/godot-orchestrator/commit/e20bfc74ac86c57481c3fd4a806c6dd181f6d473) - Update README.md
- [4b33aa6](http://github.com/Vahera/godot-orchestrator/commit/4b33aa6aa9173d791cfe3d050ef58c9a061bcbf3) - Update README.md
- [7001d13](http://github.com/Vahera/godot-orchestrator/commit/7001d1372606c41e9172c9b9c89088a14a05444d) - GH-26 Fix when removing unconnected show message choices
- [26be9f4](http://github.com/Vahera/godot-orchestrator/commit/26be9f433d4173a2c57a25df5480ea3f6bea46fc) - GH-21 Show character name field by default
- [277fbd8](http://github.com/Vahera/godot-orchestrator/commit/277fbd8a9ea5700ec9af148464bc1da2088cf825) - GH-21 Use correct speaker name for show message node
- [0b6cf15](http://github.com/Vahera/godot-orchestrator/commit/0b6cf15f2aba769d4fd1e1cc6ef753721588b3a1) - Add feature request/enhancement issues option
- [027d13b](http://github.com/Vahera/godot-orchestrator/commit/027d13bdf77aa926ce3c143299eeda0676b1b4ae) - Update config.yml

## [1.0.5](https://github.com/Vahera/godot-orchestrator/releases/tag/v1.0.5) - 2023-09-17

- [0cb4a85](http://github.com/Vahera/godot-orchestrator/commit/0cb4a854542f3afffec2b35b241d0ab06a6b677e) - Document wait-for-input-action (#14)
- [0ee6ec3](http://github.com/Vahera/godot-orchestrator/commit/0ee6ec38a641b00a487a7e217620a84c0f079ff3) - wait between each action when chaining action multiple times
- [db147c4](http://github.com/Vahera/godot-orchestrator/commit/db147c407805a1914b37d7faf33e230da4d4fc09) - adjusted node ui size
- [eacbb2e](http://github.com/Vahera/godot-orchestrator/commit/eacbb2ee16a889ccab65373b35af28d7b4d88ced) - renamed shadow variable
- [750e959](http://github.com/Vahera/godot-orchestrator/commit/750e9598580f5b2733141a24f987a9be401b34ae) - added wait for action node
- [3fb7b41](http://github.com/Vahera/godot-orchestrator/commit/3fb7b41c8282c3eff27d36d7ab4bc44593bba496) - Fix more load order dependency failures
- [402152d](http://github.com/Vahera/godot-orchestrator/commit/402152de0606cc6fad34223c849603c40d74183c) - ShowMessage: ESC/SPACE now auto-completes text
- [f585b99](http://github.com/Vahera/godot-orchestrator/commit/f585b99a9e5a69d10d263394555dc9b51d2e0115) - Add handles to AUTHORS.txt
- [8275543](http://github.com/Vahera/godot-orchestrator/commit/827554380df7218b3bb82bf4c2d8f8b641fe1ad6) - Source version from plugin.cfg
- [a162a9b](http://github.com/Vahera/godot-orchestrator/commit/a162a9b156bb6bba82528b5ca365e88e4e2a68d6) - Add GitHub actions & dependabot


## [1.0.4](https://github.com/Vahera/godot-orchestrator/releases/tag/v1.0.4) - 2023-09-16

- [4f73e27](https://github.com/Vahera/godot-orchestrator/tree/4f73e27eb97f5d20dc89a5685140db846a98a085) - Prepare 1.0.4 release (update version)


## [1.0.3](https://github.com/Vahera/godot-orchestrator/releases/tag/v1.0.3) - 2023-09-16

- [4c14f2f](https://github.com/Vahera/godot-orchestrator/tree/4c14f2ff1e4c945c3e05053e7b4fd7d2b7e0be08) - Update open file references when moving files
- [7cfc9aa](https://github.com/Vahera/godot-orchestrator/tree/7cfc9aa06b1eaad12fd4903517c2f2999cf4c505) - Cleanup editor runtime output
- [fba16d4](https://github.com/Vahera/godot-orchestrator/tree/fba16d4289f631369469673e87f162e0b42e22b0) - Fix running orchestrations in editor
- [6c30b52](https://github.com/Vahera/godot-orchestrator/tree/6c30b5297ea33742ab13d0a832b8c6554ece64b2) - Allow multiple end nodes
- [b18a349](https://github.com/Vahera/godot-orchestrator/tree/b18a34924fc749df2fb2d3fbff82125659d93a1a) - Rename README to README.md
- [d6d7f96](https://github.com/Vahera/godot-orchestrator/tree/d6d7f96526b69815207a556bfb40037625768ad7) - Duplicate README in addon folder
- [f6e9fd9](https://github.com/Vahera/godot-orchestrator/tree/f6e9fd9a2a88f83b737aa956b034470f8708d019) - Updated README.md


## [1.0.2](https://github.com/Vahera/godot-orchestrator/releases/tag/v1.0.2) - 2023-09-10

- [a82afa7](https://github.com/Vahera/godot-orchestrator/tree/a82afa7853eff1eb8250b6e3a1d5a1e401787a2b) - Save orchestrations on EditorPlugin apply_changes callback
- [33a1392](https://github.com/Vahera/godot-orchestrator/tree/33e1392f63ad8e288ddf6bc990a6c6b22a2d33bc) - Fix plug-in load order, removing const preloads
- [3b54509](https://github.com/Vahera/godot-orchestrator/tree/3b54509912d13ac0cc3daf37ba2d202f0cca5dd3) - Load resource scripts safely to avoid runtime errors


## [1.0.1](https://github.com/Vahera/godot-orchestrator/releases/tag/v1.0.1) - 2023-09-08

- Removed unused logos.


## [1.0.0](https://github.com/Vahera/godot-orchestrator/releases/tag/v1.0.0) - 2023-08-13

- Initial release