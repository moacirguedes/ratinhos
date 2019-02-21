# Ratinhos

## Você vai precisar ter instalado

- IDE do Visual Studio Community 2017 www.visualstudio.microsoft.com
    - Na instalação, quando aparecer a tela para selecionar as cargas de trabalho, selecione apenas Desenvolvimento para desktop com C++.

- vcpkg https://github.com/Microsoft/vcpkg
    - No git tem o tutorial de como instalar porém vou colocar um exemplo do zero em baixo.

ps: tudo em ambiente Windows.


## Como começar

1. Realizar a instalação do VS Community 2017

2. Fazer a clonagem do vcpkg, pode deixar em qualquer lugar mas fica bacana deixar por exemplo logo na raiz do disco C.

3. Entra na pasta que foi clonada e utilizando (como adminstrador) o PowerShell ou CMD mandar o comando: 

```
bootstrap-vcpkg.bat
```

4. Agora para fazer integração com VS mandar o comando: 

```
vcpkg integrate install
```

5. Agora o VS estara funcionando junto com o vcpkg, o próximo passo é adicionar o OpenCV ao vcpkg com o comando: 

```
vcpkg install opencv[ffmpeg] --featurepackages
```

6. Adicionar para o Path nas variaveis de ambiente do Windows o caminho para a pasta bin do OpenCV dentro do vcpkg. (Exemplo da minha instalação: C:\vcpkg\packages\opencv_x86-windows\bin)
 
7. Agora clona esse projeto e reza pra rodar :chipmunk:

ps: Caso não rode ou de algum erro envolvendo a dll ffmpeg (o que talvez vai acontecer hehe), basta entrar ali na pasta onde foi adicionada no path no passo 6, e então copiar o arquivo opencv_ffmpegXXX (XXX é a versão atual instalada) e deixar ele dentro da pasta onde fica o código fonte. (Neste projeto é possivel ver o arquivo dentro da pasta, eu coloquei também dentro da Debug/Release mas se lembro bem não precisava)

ps2: desculpe pelo código horrível 😊
