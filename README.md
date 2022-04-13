#EZ-FLASH  Omega Definitive Edition Kernel

### Author's words

ZaindORp added the save backup function, skip save when holding L and PogoShell plugins support to this version.
Why ZaindORp made this? Who knows. In fact Sterophonick's version seems better and more powerful.
ZaindORp isn't really a good programmer and she just found a old EZ-Omega kernel so ZaindORp tried to renew it and port it to the EZ-Omega Defintive Edition.
But after everything is done she found erophonick's version and found the project have already be made before but decided to keep on developing ZaindORp's own version. ZaindORp has no EZ-Omega so she can't test the kernel on that and won't make a version for that.
By the way ZaindORp always call herself in the third person.

### How to build 

    1.We use devkitARM_r53, you can use the current version or newer.
    2.Set the following environment variables in system, or modify the value in build.bat, based on your installation path
 
        PATH,DEVKITARM,DEVKITPRO,LIBGBA

    3.Double click on build.bat under winodws. If it goes well, you will get ezkernelnew.gba
    4.Double click on  Link_kernel_image.exe link the ezkernelnew.gba and image.bin to ezkernelnew.bin, that is the omegaDE kernel upgrade file
