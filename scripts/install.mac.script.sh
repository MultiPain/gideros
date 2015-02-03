echo 'Updating brew'
brew update &> /dev/null
echo 'Finished updating brew'
echo 'Installing dependencies'
(brew install freetype &> /dev/null) &
(brew install glew &> /dev/null) &
(brew install qt5 &> /dev/null) &
(brew install ant &> /dev/null) &
(brew install android-sdk &> /dev/null
brew install android-ndk &> /dev/null) &
wait
echo 'Finished installing dependencies'
expect -c '
set timeout -1   ;
spawn android update sdk -u -t platform-tool,tool,build-tools-21.0.2,android-21; 
expect { 
    "Do you accept the license" { exp_send "y\r" ; exp_continue }
    eof
}
'

export QT=/usr/local/Cellar/qt5/5.4.0
export QT_WIN=~/.wine/drive_c/Qt/Qt5.3.2
export IOS_SDK=8.1
export ANDROID_HOME=/usr/local/opt/android-sdk
export ANDROID_NDK=/usr/local/opt/android-ndk

rm -rf build
mkdir build

cd scripts
(
echo 'Building iOS libraries...'
bash cleanioslibs.sh &> /dev/null
bash buildioslibs.sh &> /dev/null
bash buildiosplugins.sh &> /dev/null
) &
(
echo 'Building Android libraries...'
bash makejar.sh
bash buildandroidlibs.sh
bash buildandroidso.sh
bash buildandroidplugins.sh
) &
(
echo 'Installing QScintilla for Mac...'
bash downloadqscintilla.sh &> /dev/null
bash extractqscintilla.sh &> /dev/null
bash installqscintilla.sh &> /dev/null

echo 'Building Qt applications for Mac...'
rm -rf ../Sdk
bash qt5/buildqtlibs.sh
bash qt5/buildplugins.sh
bash qt5/cleanqt.sh
bash qt5/buildqt.sh
) &
wait

echo 'Copying Mac files...'
bash copymac.sh

echo 'Creating Mac installation package...'
bash createmacpackage.sh



