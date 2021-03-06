/***************************************************************************
 *   Copyright (C) 2013 by dcblaha,,,   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "ComponentFinder.h"
#include "srcFileParser.h"
#include "ComponentBuilder.h"
#include "Version.h"
#include "BuildConfigWriter.h"
#include "Project.h"
#include "Packages.h"
#include "Coverage.h"
#include "OovError.h"
#include <stdio.h>


class OovBuilder
    {
    public:
        void process(eProcessModes processMode, OovStringRef oovProjDir,
            OovStringRef buildConfigName, bool verbose);

    private:
        ComponentFinder mCompFinder;

        void analyze(BuildConfigWriter &cfg, eProcessModes procMode,
            OovStringRef const buildConfigName, OovStringRef const srcRootDir);
        void build(eProcessModes processMode, OovStringRef oovProjDir,
            OovStringRef buildConfigName, bool verbose);
        void clean(eProcessModes pm, OovStringRef oovProjDir);
        bool readProject(OovStringRef oovProjDir, OovStringRef buildMode,
            OovStringRef buildConfigName, bool verbose);
        ComponentFinder &getComponentFinder()
            { return mCompFinder; }
    };

void OovBuilder::process(eProcessModes processMode, OovStringRef oovProjDir,
    OovStringRef buildConfigName, bool verbose)
    {
    if(processMode & PM_CleanMask)
        {
        clean(processMode, oovProjDir);
        }
    else
        {
        build(processMode, oovProjDir, buildConfigName, verbose);
        }
    }

static OovStatusReturn cleanMatchingDir(OovStringRef path)
    {
    std::vector<std::string> dirs;
    OovStatus status = getDirListMatch(path, dirs);
    if(status.ok())
        {
        for(auto const &dir : dirs)
            {
            status = recursiveDeleteDir(dir);
            if(!status.ok())
                {
                break;
                }
            }
        }
    return status;
    }

void OovBuilder::clean(eProcessModes pm, OovStringRef oovProjDir)
    {
    OovStatus status(true, SC_File);
    Project::setProjectDirectory(oovProjDir);
    if(pm & PM_CleanAnalyze)
        {
        printf("Cleaning Analyze\n");
        FilePath analysisPath(oovProjDir, FP_Dir);
        analysisPath.appendFile(BuildConfig::getBaseAnalysisPath());
        analysisPath.appendFile("*");
        status = cleanMatchingDir(analysisPath);
        if(status.ok())
            {
            status = recursiveDeleteDir(Project::getOutputDir());
            }
        }
    if(pm & PM_CleanCoverage)
        {
        printf("Cleaning Coverage\n");
        if(status.ok())
            {
            if(FileIsDirOnDisk(Project::getCoverageSourceDirectory(), status))
                {
                status = recursiveDeleteDir(Project::getCoverageSourceDirectory());
                }
            }
        if(status.ok())
            {
            if(FileIsDirOnDisk(Project::getCoverageProjectDirectory(), status))
                {
                status = recursiveDeleteDir(Project::getCoverageProjectDirectory());
                }
            }
        }
    if(pm & PM_CleanBuild)
        {
        printf("Cleaning Build\n");
        if(status.ok())
            {
            FilePath buildIntermediatePath(oovProjDir, FP_Dir);
            buildIntermediatePath.appendFile("bld-*");
            status = cleanMatchingDir(buildIntermediatePath);
            }
        if(status.ok())
            {
            FilePath buildOutputPath(oovProjDir, FP_Dir);
            buildOutputPath.appendFile("out-*");
            status = cleanMatchingDir(buildOutputPath);
            }
        }
    if(status.ok())
        {
        printf("Cleaning tmp\n");
        FilePath analysisPath(oovProjDir, FP_Dir);
        analysisPath.appendFile("oovaide-tmp-*");
        std::vector<std::string> files;
        status = getDirListMatch(analysisPath, files);
        if(status.ok())
            {
            for(auto const &file : files)
                {
                status = FileDelete(file);
                if(!status.ok())
                    {
                    break;
                    }
                }
            }
        }
    if(status.needReport())
        {
        status.report(ET_Error, "Unable to clean");
        }
    }

void OovBuilder::build(eProcessModes processMode, OovStringRef oovProjDir,
        OovStringRef buildConfigName, bool verbose)
    {
    bool success = true;
    Project::setProjectDirectory(oovProjDir);
    if(success)
        {
        OovString buildMode;
        switch(processMode)
            {
            case PM_Analyze:        buildMode = OptFilterValueBuildModeAnalyze;       break;
            default:
            case PM_Build:          buildMode = OptFilterValueBuildModeBuild;         break;
            }
        if(processMode == PM_CovBuild)
            {
            success = makeCoverageBuildProject(mCompFinder.getProject());
            }
        if(success)
            {
            // This must be after creating the coverage project.
            success = readProject(Project::getProjectDirectory(), buildMode,
                buildConfigName, verbose);
            }
        }
    if(success)
        {
        if(processMode == PM_CovStats)
            {
            if(makeCoverageStats())
                {
                printf("Coverage output: %s",
                    Project::getCoverageProjectDirectory().getStr());
                }
            }
        else
            {
            BuildConfigWriter cfg;
            analyze(cfg, processMode, buildConfigName, Project::getSrcRootDirectory());

            std::string configStr = "Configuration: ";
            configStr += buildConfigName;
            sVerboseDump.logProgress(configStr);
            switch(processMode)
                {
                case PM_Analyze:
                    break;

                default:
                    {
                    std::string incDepsPath = cfg.getIncDepsFilePath();
                    ComponentBuilder compBuilder(getComponentFinder());
                    compBuilder.build(processMode, incDepsPath, buildConfigName);
                    }
                    break;
                }
            }
        }
    }

bool OovBuilder::readProject(OovStringRef oovProjDir, OovStringRef buildMode,
    OovStringRef buildConfigName, bool verbose)
    {
    bool success = mCompFinder.readProject(oovProjDir, buildMode, buildConfigName);
    if(success)
        {
        mCompFinder.getProjectBuildArgs().setCompConfig("");    // This updates verbose arg
        if(mCompFinder.getProjectBuildArgs().getVerbose() || verbose)
            {
            sVerboseDump.open(oovProjDir);
            }
        }
    else
        {
        fprintf(stderr, "oovBuilder: Oov project file must exist in %s\n",
            oovProjDir.getStr());
        }
    return success;
    }

void OovBuilder::analyze(BuildConfigWriter &cfg,
        eProcessModes procMode, OovStringRef const buildConfigName,
        OovStringRef const srcRootDir)
    {
    // @todo - This should probably check the oovaide-pkg.txt include crc to
    // see if it is different than what was used to generate the
    // oovaide-tmp-buildpkg.txt.  This cheat may be ok, but seems to
    // work a bit strange that sometimes the buildpkg file is not recreated
    // immediately after the oovaide-pkg.txt file is updated.  Also, the
    // analysis directory should probably be deleted if the include
    // paths have changed.
    OovString bldPkgFilename = Project::getBuildPackagesFilePath();
    OovStatus status(true, SC_File);
    bool havePackages = FileIsFileOnDisk(bldPkgFilename, status);
    if(havePackages)
        {
        if(FileStat::isOutputOld(bldPkgFilename, ProjectPackages::getFilename(),
            status))
            {
            printf("Deleting build config\n");
            if(status.ok())
                {
                status = FileDelete(bldPkgFilename.getStr());
                }
            if(status.ok())
                {
                status = FileDelete(cfg.getBuildConfigFilename().c_str());
                }
            havePackages = false;
            }
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to clean up packages");
            }
        }
    if(status.needReport())
        {
        OovString str = "Unable to check build package file: ";
        str += bldPkgFilename;
        status.report(ET_Error, str);
        }

    cfg.setInitialConfig(buildConfigName,
        mCompFinder.getProjectBuildArgs().getExternalArgs(),
        mCompFinder.getProjectBuildArgs().getAllCrcCompileArgs(),
        mCompFinder.getProjectBuildArgs().getAllCrcLinkArgs());

    if(status.ok() && (cfg.isConfigDifferent(buildConfigName, BuildConfig::CT_ExtPathArgsCrc) ||
            !havePackages))
        {
        // This is for the -ER switch.
        for(auto const &arg : mCompFinder.getProjectBuildArgs().getExternalArgs())
            {
            printf("Scanning %s\n", &arg[3]);
            fflush(stdout);
            mCompFinder.scanExternalProject(&arg[3]);
            }
        for(auto const &pkg : mCompFinder.getProjectBuildArgs().getProjectPackages().
            getPackages())
            {
            if(!mCompFinder.doesBuildPackageExist(pkg.getPkgName()))
                {
                if(pkg.needDirScan())
                    {
                    printf("Scanning %s\n", pkg.getPkgName().getStr());
                    fflush(stdout);
                    mCompFinder.scanExternalProject(pkg.getRootDir(), &pkg);
                    }
                else
                    {
                    mCompFinder.addBuildPackage(pkg);
                    }
                }
            }
        }
    if(status.ok())
        {
        printf("Scanning %s\n", srcRootDir.getStr());
        status = mCompFinder.scanProject();
        fflush(stdout);
        }
    if(status.ok())
        {
        srcFileParser sfp(mCompFinder);
        cfg.setProjectConfig(mCompFinder.getScannedInfo().getProjectIncludeDirs());
        // Is anything different in the current build configuration?
        if(cfg.isAnyConfigDifferent(buildConfigName))
            {
            // Find CRC's that are not used by any build configuration.
            OovStringVec unusedCrcs;
            OovStringVec deleteDirs;
            cfg.getUnusedCrcs(unusedCrcs);
            for(auto const &crcStr : unusedCrcs)
                {
                deleteDirs.push_back(cfg.getAnalysisPathUsingCRC(crcStr));
                }
            bool analysisDifferent = (cfg.isConfigDifferent(buildConfigName,
                BuildConfig::CT_ExtPathArgsCrc) ||
                cfg.isConfigDifferent(buildConfigName,
                BuildConfig::CT_ProjPathArgsCrc));
            bool buildDifferent = (analysisDifferent ||
                cfg.isConfigDifferent(buildConfigName,
                BuildConfig::CT_OtherArgsCrc));
            if(buildDifferent)
                {
                if(procMode != PM_Analyze)
                    {
                    deleteDirs.push_back(Project::getIntermediateDir(buildConfigName));
                    }
                }
            else    // Must be only link arguments.
                {
                deleteDirs.push_back(Project::getBuildOutputDir(buildConfigName));
                }
            cfg.saveConfig(buildConfigName);
    // This isn't needed because the CRC will be different and save the analysis
    // results to a different directory.
    /*
            // This must be after the saveConfig, because it uses the new CRC's
            // to delete the new analysis path.
            OovString analysisPath = cfg.getAnalysisPath();
            if(analysisDifferent)
                {
                deleteDirs.push_back(analysisPath);
                }
    */
            for(auto const &dir : deleteDirs)
                {
                printf("Deleting %s\n", dir.getStr());
                if(dir.length() > 5)        // Reduce chance of deleting root
                    {
                    if(FileIsDirOnDisk(dir, status))
                        {
                        status = recursiveDeleteDir(dir);
                        }
                    if(status.needReport())
                        {
                        status.report(ET_Error, "Unable to clean up directories");
                        }
                    }
                }
            fflush(stdout);
    // See comment above.
    /*
            if(analysisDifferent)
                {
                // Windows returns before the directory is actually deleted.
                FileWaitForDirDeleted(analysisPath);
                }
    */
            }

        OovString analysisPath = cfg.getAnalysisPath();
        status = FileEnsurePathExists(analysisPath);
        if(status.ok())
            {
            mCompFinder.saveProject(analysisPath);
            sfp.analyzeSrcFiles(srcRootDir, analysisPath);
            }
        if(status.needReport())
            {
            status.report(ET_Error, "Unable to create analysis path");
            }
        }
    }

// Example test args:
//      ../examples/staticlib ../examples/staticlib-oovaide -bld-Debug
int main(int argc, char const * const argv[])
    {
    char const *oovProjDir = NULL;
    OovBuilder builder;
    std::string buildConfigName = BuildConfigAnalysis;  // analysis is default.
    eProcessModes processMode = PM_Analyze;
    OovError::setComponent(EC_OovBuilder);
    bool verbose = false;
    bool success = (argc >= 2);
    if(success)
        {
        oovProjDir = argv[1];
        for(int i=2; i<argc; i++)
            {
            std::string testArg = argv[i];
            if(testArg.find("-cfg-", 0, 5) == 0)
                {
                buildConfigName = testArg.substr(5);    // skip "-cfg-"
                }
            else if(testArg.find("-mode-", 0, 5) == 0)
                {
                OovString mode;
                mode.setLowerCase(testArg.substr(6));
                if(mode.find("clean-") == 0)
                    {
                    processMode = PM_None;
                    for(size_t pos = mode.find("-")+1; pos < mode.length(); pos++)
                        {
                        if(mode[pos] == 'a')
                            {
                            processMode = static_cast<eProcessModes>(
                                processMode | PM_CleanAnalyze);
                            }
                        if(mode[pos] == 'b')
                            {
                            processMode = static_cast<eProcessModes>(
                                processMode | PM_CleanBuild);
                            }
                        if(mode[pos] == 'c')
                            {
                            processMode = static_cast<eProcessModes>(
                                processMode | PM_CleanCoverage);
                            }
                        }
                    }
                else if(mode.find("cov-instr") == 0)
                    {
                    processMode = PM_CovInstr;
                    }
                else if(mode.find("cov-build") == 0)
                    {
                    processMode = PM_CovBuild;
                    }
                else if(mode.find("analyze") == 0)
                    {
                    processMode = PM_Analyze;
                    }
                else if(mode.find("cov-stat") == 0)
                    {
                    processMode = PM_CovStats;
                    }
                else if(mode.find("build") == 0)
                    {
                    processMode = PM_Build;
                    }
                }
            else if(testArg.compare("-bv") == 0)
                {
                verbose = true;
                }
            }
        }
    else
        {
        fprintf(stderr, "OovBuilder version %s\n", OOV_VERSION);
            fprintf(stderr, "Command format:    <oovProjectDir> [args]...\n");
            fprintf(stderr, "  The oovProjectDir can have exclusion paths using <oovProjectDir>!<path> \n");
            fprintf(stderr, "  The args are:\n");
            fprintf(stderr, "    -cfg-<buildconfig>\n");
            fprintf(stderr, "               buildconfig is Debug, Release or any custom name\n");
            fprintf(stderr, "    -mode-<analyze|build|clean-[abc]|cov-instr|cov-build|cov-stats>\n");
            fprintf(stderr, "               cov means coverage, [abc] means analyze, build, coverage \n");
            fprintf(stderr, "    -bv         builder verbose - OovBuilder.txt file\n");
        }

    if(success)
        {
        builder.process(processMode, oovProjDir, buildConfigName, verbose);
        }
    return 0;
    }

