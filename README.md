# Unreal Example
언리얼 사용 예제 모음

## Note
https://docs.unrealengine.com/5.0/ko/plugins-in-unreal-engine/
플러그인 구성 파일은 프로젝트와 같이 패키징되지 않습니다. 
앞으로 지원될 수는 있지만, 현재 파일을 프로젝트의 Config 폴더에 수동 복사해야 합니다.

Descripter (.uplugin)
https://docs.unrealengine.com/5.0/en-US/API/Runtime/Projects/FPluginDescriptor/

Modules
https://docs.unrealengine.com/5.0/en-US/API/Runtime/Projects/FModuleDescriptor/

Type
https://docs.unrealengine.com/5.0/en-US/API/Runtime/Projects/EHostType__Type/

LoadingPhase
https://docs.unrealengine.com/5.0/en-US/API/Runtime/Projects/ELoadingPhase__Type/

Icon
디스크립터 파일과 함께 플러그인은 에디터 플러그인 브라우저에 표시할 아이콘이 필요합니다. 
이미지는 128x128 .png 파일로 이름은 "Icon128.png" 여야 하며 플러그인의 "/Resources/" 디렉터리에 있어야 합니다.
