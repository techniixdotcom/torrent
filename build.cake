#addin "nuget:?package=Cake.CMake&version=1.4.0"
#tool  "nuget:?package=wix&version=3.14.1"

//////////////////////////////////////////////////////////////////////
// ARGUMENTS
//////////////////////////////////////////////////////////////////////

var target        = Argument("target", "Default");
var configuration = Argument("configuration", "Release");
var platform      = Argument("platform", "x64");
var vcpkgTriplet  = Argument("vcpkg-triplet", "x64-windows-static-md");

// Parameters
var OutputDirectory       = Directory("./build-" + platform);
var BuildDirectory        = OutputDirectory + Directory(configuration);
var PackagesDirectory     = BuildDirectory + Directory("packages");
var PublishDirectory      = BuildDirectory + Directory("publish");
var BootstrapperDirectory = Directory("./src/installer/bin") + Directory(configuration);
var ResourceDirectory     = Directory("./res");

var Version            = Argument("build-version", "1.0.0");
var VersionParts       = Version.Split('.');
var VersionMajor       = VersionParts.Length > 0 ? VersionParts[0] : "0";
var VersionMinor       = VersionParts.Length > 1 ? VersionParts[1] : "0";
var VersionPatch       = VersionParts.Length > 2 ? VersionParts[2] : "0";
var BranchName         = Argument("build-branch", "main");
var ShortSha           = Argument("build-sha", "0000000");
var Installer          = string.Format("NiiXTorrent-{0}-{1}.msi", Version, platform);
var InstallerBundle    = string.Format("NiiXTorrent-{0}-{1}.exe", Version, platform);
var PortablePackage    = string.Format("NiiXTorrent-{0}-{1}.zip", Version, platform);
var SymbolsPackage     = string.Format("NiiXTorrent-{0}-{1}.symbols.zip", Version, platform);

//////////////////////////////////////////////////////////////////////
// TASKS
//////////////////////////////////////////////////////////////////////

Task("Clean")
    .Does(() =>
{
    CleanDirectory(BootstrapperDirectory);
    CleanDirectory(BuildDirectory);
});

Task("Generate-Project")
    .IsDependentOn("Clean")
    .Does(() =>
{
    CMake(new CMakeSettings
    {
        SourcePath = ".",
        OutputPath = OutputDirectory,
        Generator = "Visual Studio 18 2026",
        Platform = platform == "x86" ? "Win32" : "x64",
        Toolset = "v145",
        Options = new []
        {
            $"-DGITVERSION_VAR_BRANCHNAME={BranchName}",
            $"-DGITVERSION_VAR_SEMVER={Version}",
            $"-DGITVERSION_VAR_SHORTSHA={ShortSha}",
            $"-DGITVERSION_VAR_VERSION_MAJOR={VersionMajor}",
            $"-DGITVERSION_VAR_VERSION_MINOR={VersionMinor}",
            $"-DGITVERSION_VAR_VERSION_PATCH={VersionPatch}",
            $"-DGITVERSION_VAR_VERSION={Version}",
            $"-DVCPKG_TARGET_TRIPLET={vcpkgTriplet}"
        }
    });
});

Task("Build")
    .IsDependentOn("Generate-Project")
    .Does(() =>
{
    var settings = new MSBuildSettings()
        .SetConfiguration(configuration)
        .SetMaxCpuCount(0)
        .UseToolVersion(MSBuildToolVersion.Default);
    if(platform == "x86")
    {
        settings.WithProperty("Platform", "Win32")
                .SetPlatformTarget(PlatformTarget.x86);
    }
    else
    {
        settings.WithProperty("Platform", "x64")
                .SetPlatformTarget(PlatformTarget.x64);
    }

    MSBuild(OutputDirectory + File("NiiXTorrent.vcxproj"), settings);

    // Plugins
    MSBuild(OutputDirectory + File("Plugin_Updater.vcxproj"), settings);
});

Task("Setup-Publish-Directory")
    .IsDependentOn("Build")
    .Does(() =>
{
    CreateDirectory(PublishDirectory);

    CopyFileToDirectory(MakeAbsolute(BuildDirectory + File("NiiXTorrent.exe")), PublishDirectory);
    CopyFileToDirectory(MakeAbsolute(BuildDirectory + File("coredb.sqlite")), PublishDirectory);
    CopyFileToDirectory(MakeAbsolute(BuildDirectory + File("Plugin_Updater.dll")), PublishDirectory);

    // crashpad_handler.exe only exists when the build includes crash reporting.
    var crashpad = MakeAbsolute(BuildDirectory + File("crashpad_handler.exe"));
    if (FileExists(crashpad))
    {
        CopyFileToDirectory(crashpad, PublishDirectory);
    }
});

Task("Build-Installer")
    .IsDependentOn("Build")
    .IsDependentOn("Setup-Publish-Directory")
    .Does(() =>
{
    var arch = Architecture.X64;

    if(platform == "x86")
    {
        arch = Architecture.X86;
    }

    var sourceFiles = new FilePath[]
    {
        "./packaging/WiX/NiiXTorrent.wxs",
        "./packaging/WiX/NiiXTorrent.Components.wxs",
        "./packaging/WiX/NiiXTorrent.Directories.wxs"
    };

    var objFiles = new FilePath[]
    {
        BuildDirectory + File("NiiXTorrent.wixobj"),
        BuildDirectory + File("NiiXTorrent.Components.wixobj"),
        BuildDirectory + File("NiiXTorrent.Directories.wixobj")
    };

    // crashpad_handler.exe is only present when crash reporting is built in.
    var withCrashpad = FileExists(MakeAbsolute(PublishDirectory + File("crashpad_handler.exe"))) ? "yes" : "no";

    WiXCandle(sourceFiles, new CandleSettings
    {
        Architecture = arch,
        Defines = new Dictionary<string, string>
        {
            { "Configuration", configuration },
            { "PublishDirectory", PublishDirectory },
            { "Platform", platform },
            { "ResourceDirectory", ResourceDirectory },
            { "Version", Version },
            { "WithCrashpad", withCrashpad }
        },
        Extensions = new [] { "WixFirewallExtension", "WixUtilExtension" },
        OutputDirectory = BuildDirectory
    });

    WiXLight(objFiles, new LightSettings
    {
        ArgumentCustomization = args =>
            args.Append("-sice:ICE38")
                .Append("-sice:ICE91"),
        Extensions = new [] { "WixFirewallExtension", "WixUtilExtension" },
        OutputFile = PackagesDirectory + File(Installer)
    });
});

Task("Build-Installer-Bootstrapper")
    .Does(() =>
{
    var settings = new MSBuildSettings()
        .SetConfiguration(configuration)
        .SetMaxCpuCount(0)
        .UseToolVersion(MSBuildToolVersion.Default);

    MSBuild("./src/installer/NiiXTorrentBootstrapper.sln", settings);
});

Task("Build-Installer-Bundle")
    .IsDependentOn("Build-Installer")
    .IsDependentOn("Build-Installer-Bootstrapper")
    .Does(() =>
{
    var arch = Architecture.X64;

    if(platform == "x86")
    {
        arch = Architecture.X86;
    }

    WiXCandle("./packaging/WiX/NiiXTorrentBundle.wxs", new CandleSettings
    {
        Architecture = arch,
        Extensions = new [] { "WixBalExtension", "WixNetFxExtension", "WixUtilExtension" },
        Defines = new Dictionary<string, string>
        {
            { "NiiXTorrentInstaller", PackagesDirectory + File(Installer) },
            { "Platform", platform },
            { "Version", Version }
        },
        OutputDirectory = BuildDirectory
    });

    WiXLight((BuildDirectory + File("NiiXTorrentBundle.wixobj")).ToString(), new LightSettings
    {
        Extensions = new [] { "WixBalExtension", "WixNetFxExtension", "WixUtilExtension" },
        OutputFile = PackagesDirectory + File(InstallerBundle)
    });
});

Task("Build-Portable-Package")
    .IsDependentOn("Build")
    .IsDependentOn("Setup-Publish-Directory")
    .Does(() =>
{
    Zip(PublishDirectory, PackagesDirectory + File(PortablePackage));
});

Task("Build-Symbols-Package")
    .IsDependentOn("Build")
    .Does(() =>
{
    var files = new FilePath[]
    {
        BuildDirectory + File("NiiXTorrent.pdb"),
    };

    Zip(BuildDirectory, PackagesDirectory + File(SymbolsPackage), files);
});

//////////////////////////////////////////////////////////////////////
// TASK TARGETS
//////////////////////////////////////////////////////////////////////

Task("Default")
    .IsDependentOn("Build")
    ;

Task("Publish")
    .IsDependentOn("Build")
    .IsDependentOn("Build-Installer")
    .IsDependentOn("Build-Installer-Bundle")
    .IsDependentOn("Build-Portable-Package")
    .IsDependentOn("Build-Symbols-Package");

//////////////////////////////////////////////////////////////////////
// EXECUTION
//////////////////////////////////////////////////////////////////////

RunTarget(target);
