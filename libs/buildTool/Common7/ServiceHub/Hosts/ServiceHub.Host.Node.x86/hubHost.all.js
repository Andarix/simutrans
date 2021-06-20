(function(){function r(e,n,t){function o(i,f){if(!n[i]){if(!e[i]){var c="function"==typeof require&&require;if(!f&&c)return c(i,!0);if(u)return u(i,!0);var a=new Error("Cannot find module '"+i+"'");throw a.code="MODULE_NOT_FOUND",a}var p=n[i]={exports:{}};e[i][0].call(p.exports,function(r){var n=e[i][1][r];return o(n||r)},p,p.exports,r,e,n,t)}return n[i].exports}for(var u="function"==typeof require&&require,i=0;i<t.length;i++)o(t[i]);return o}return r})()({1:[function(require,module,exports){
"use strict";
var net = require('net');
var path = require('path');
var os = require('os');
var fs = require('fs');
var q = require('q');
var ec = require('errno-codes');
var mkdirp = require('mkdir-parents');
var rmdirRecursive = require('rmdir-recursive');
var sha256 = require('js-sha256').sha256;
var randomHexString_1 = require('./randomHexString');
var libraryResourceStrings_1 = require('./libraryResourceStrings');
var report_errors_1 = require('report-errors');
var tmp = require('tmp');
var CurrentOsLocaleEnvVarName = 'ServiceHubCurrentOsLocale';
// Keep in sync with the env var under src\clr\utility\Microsoft.ServiceHub.Utility.Shared\EnvUtils.cs
exports.UniqueLogSessionKeyEnvVarName = 'ServiceHubLogSessionKey';
// Enable long stack support for Q promises
// TODO: revisit. Long stack support in Q may impose some perf costs if there are many promises or many users.
q.longStackSupport = true;
/**
 * Sets OS locale to Hub host.
 */
function setOsLocale(localeConfigs) {
    // Default to English US if an older version of the controller that does not set the locale env var was used
    var locale = 'en-US';
    if (process.env[CurrentOsLocaleEnvVarName]) {
        locale = process.env[CurrentOsLocaleEnvVarName];
    }
    localeConfigs.forEach(function (config) {
        config(locale, /* cache */ true);
    });
}
exports.setOsLocale = setOsLocale;
;
function registerForCrashReporting(crashFileLogPrefix) {
    var errorHandler = new report_errors_1.ErrorHandlerNode({ appRoot: __dirname, packageInfo: require(path.join(__dirname, 'package.json')) });
    // Write failures to a log file
    var crashLogFilePrefix = tmp.tmpNameSync({ template: crashFileLogPrefix + "-" + process.pid + "-XXXXXX", postfix: '.log' });
    errorHandler.addReportingChannel(new report_errors_1.FileReporter(getLogFilesDir(), crashLogFilePrefix + ".log", /* stripStack */ true));
}
exports.registerForCrashReporting = registerForCrashReporting;
/* ServiceHub error classes */
(function (ErrorKind) {
    ErrorKind[ErrorKind["Error"] = 0] = "Error";
    ErrorKind[ErrorKind["InvalidArgument"] = 1] = "InvalidArgument";
    ErrorKind[ErrorKind["InvalidOperation"] = 2] = "InvalidOperation";
    ErrorKind[ErrorKind["ConfigurationError"] = 3] = "ConfigurationError";
    ErrorKind[ErrorKind["ServiceModuleInfoNotFound"] = 4] = "ServiceModuleInfoNotFound";
    ErrorKind[ErrorKind["ServiceModuleInfoLoadError"] = 5] = "ServiceModuleInfoLoadError";
    ErrorKind[ErrorKind["ServiceModuleInfoInvalidPropertyError"] = 6] = "ServiceModuleInfoInvalidPropertyError";
    ErrorKind[ErrorKind["HubHostInfoLoadError"] = 7] = "HubHostInfoLoadError";
    ErrorKind[ErrorKind["JsonParseError"] = 8] = "JsonParseError";
    ErrorKind[ErrorKind["HostGroupsNotSupported"] = 9] = "HostGroupsNotSupported";
    ErrorKind[ErrorKind["ObjectDisposed"] = 10] = "ObjectDisposed";
})(exports.ErrorKind || (exports.ErrorKind = {}));
var ErrorKind = exports.ErrorKind;
// Default error messages for the cases where caller can pass only error kind to ServiceHubError ctor.
var defaultErrorMessages = {};
var ServiceHubError = (function () {
    function ServiceHubError(kind, message) {
        this.name = 'ServiceHubError';
        this.kind = kind || ErrorKind.Error;
        this.message = message;
        if (!this.message) {
            ServiceHubError.initDefaultErrorMessages();
            this.message = defaultErrorMessages[this.kind] || ErrorKind[this.kind];
        }
        this.stack = (new Error).stack;
    }
    ServiceHubError.initDefaultErrorMessages = function () {
        if (!ServiceHubError.areDefaultErrorMessagesInitialized) {
            defaultErrorMessages[ErrorKind.ObjectDisposed] = libraryResourceStrings_1.LibraryResourceStrings.objectDisposed;
            ServiceHubError.areDefaultErrorMessagesInitialized = true;
        }
    };
    return ServiceHubError;
}());
exports.ServiceHubError = ServiceHubError;
var Constants = (function () {
    function Constants() {
    }
    Constants.HostInfoFolderName = 'host';
    Constants.HubHostScript = 'HubHost.js';
    Constants.PipeScheme = 'net.pipe://';
    return Constants;
}());
exports.Constants = Constants;
var Server = (function () {
    function Server() {
    }
    return Server;
}());
var ServerManager = (function () {
    function ServerManager() {
        this.activeStreams = [];
        this.activeServers = [];
    }
    // Returns the endpoint that people connect to in order to have an instance of 'name' created and hooked up to the proper callback stream
    ServerManager.prototype.startService = function (options, connectionCallback) {
        var _this = this;
        if (!options) {
            throw new ServiceHubError(ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variablesAreNotDefined('options'));
        }
        if (!options.logger) {
            throw new ServiceHubError(ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variableIsNotDefined('options.logger'));
        }
        var server = new Server();
        var serverOnConnectionCallback = null;
        var url;
        serverOnConnectionCallback = function (stream) {
            server.server.close();
            _this.activeStreams.push(stream);
            connectionCallback(stream, options.logger);
        };
        var netServer = net.createServer(serverOnConnectionCallback);
        server.server = netServer;
        this.activeServers.push(server);
        // Start the pipe listener
        return q.resolve(options.pipeName || randomHexString_1.default())
            .then(function (pipeName) {
            pipeName = formatPipeName(pipeName);
            return getServerPipePath(pipeName)
                .then(function (serverPath) {
                var url = Constants.PipeScheme + pipeName;
                options.logger.info(libraryResourceStrings_1.LibraryResourceStrings.startingServer(options.name, url, serverPath));
                server.url = url;
                if (options.onErrorCb) {
                    server.server.on('error', options.onErrorCb);
                }
                q.ninvoke(server.server, 'once', 'close').done(function () {
                    options.logger.info(libraryResourceStrings_1.LibraryResourceStrings.stoppedServer(options.name, url));
                    _this.activeServers.splice(_this.activeServers.indexOf(server), 1);
                });
                server.server.listen(serverPath);
                return url;
            });
        });
    };
    ServerManager.prototype.cancelServiceRequest = function (url) {
        for (var i = 0; i < this.activeServers.length; i++) {
            if (this.activeServers[i].url == url) {
                this.activeServers[i].server.close();
                break;
            }
        }
    };
    ServerManager.prototype.stop = function () {
        if (!this.isClosed) {
            this.isClosed = true;
            var promises_1 = [];
            this.activeServers.forEach(function (server) { return promises_1.push(q.ninvoke(server.server, 'close')); });
            return q.all(promises_1);
        }
        // The server has already been stopped, so no-op
        return q.resolve(function () { });
    };
    return ServerManager;
}());
exports.serverManager = new ServerManager();
function getPipeName(url) {
    if (!url) {
        throw new ServiceHubError(ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variableIsNotDefined('url'));
    }
    if (url.indexOf(Constants.PipeScheme) !== 0) {
        throw new ServiceHubError(ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.urlShouldStartWith(Constants.PipeScheme));
    }
    var pipeName = url.slice(Constants.PipeScheme.length).trim();
    if (!pipeName) {
        throw new ServiceHubError(ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.urlHasNoPipeName(Constants.PipeScheme));
    }
    return pipeName;
}
exports.getPipeName = getPipeName;
var unixSocketDir = null;
// For *nix platforms we create a socket dir that looks like this: '~/.ServiceHub/<controllerPipeName>'
// ~ avoids pipe name collisions between different users who are running apps with the same ServiceHub config file.
// <controllerPipeName> sub dir ensures each ServiceHub session gets its own dir for socket files.
function getUnixSocketDir(controllerPipeName) {
    if (unixSocketDir) {
        return q.resolve(unixSocketDir);
    }
    return q.resolve(path.join(getServiceHubBaseDirForUnix(), controllerPipeName));
}
exports.getUnixSocketDir = getUnixSocketDir;
var unixSocketDirCreated = false;
exports.unixOwnerOnlyAccessMode = parseInt('0700', 8); // 'rwx------'
function getServerPipePath(pipeNameOrUrl) {
    if (!pipeNameOrUrl) {
        throw new Error(libraryResourceStrings_1.LibraryResourceStrings.variableIsNotDefined('pipeName'));
    }
    if (pipeNameOrUrl.indexOf(Constants.PipeScheme) === 0) {
        pipeNameOrUrl = pipeNameOrUrl.slice(Constants.PipeScheme.length).trim();
        if (!pipeNameOrUrl) {
            throw new ServiceHubError(ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.noPipeName(Constants.PipeScheme));
        }
    }
    if (os.platform() === 'win32') {
        return q.resolve(path.join('\\\\?\\pipe', pipeNameOrUrl));
    }
    // Since the location service pipe is the first one opened and as soon as its opened this environment variable is set,
    // if the variable isn't set it means we're trying to get the socket dir for that pipe.
    var controllerPipeName = getLocationServicePipeName();
    if (!controllerPipeName) {
        controllerPipeName = pipeNameOrUrl;
    }
    return getUnixSocketDir(controllerPipeName)
        .then(function (socketDir) {
        var serverPipePath = path.join(socketDir, pipeNameOrUrl);
        if (unixSocketDirCreated) {
            return q.resolve(serverPipePath);
        }
        var dirCheck = q.defer();
        fs.exists(socketDir, function (exists) { return dirCheck.resolve(exists); });
        return dirCheck.promise
            .then(function (exists) {
            if (exists) {
                unixSocketDirCreated = true;
                return serverPipePath;
            }
            else {
                return q.nfcall(mkdirp, socketDir, exports.unixOwnerOnlyAccessMode)
                    .then(function () {
                    unixSocketDirCreated = true;
                    return serverPipePath;
                })
                    .catch(function (e) {
                    if (e.code !== ec.EEXIST.code) {
                        throw e;
                    }
                    unixSocketDirCreated = true;
                    return serverPipePath;
                });
            }
        });
    });
}
exports.getServerPipePath = getServerPipePath;
function deleteUnixSocketDir(url) {
    if (process.platform === 'win32') {
        return q.resolve({});
    }
    // Since the location service pipe is the first one opened and as soon as its opened this environment variable is set,
    // if the variable isn't set it means we're trying to get the socket dir for that pipe.
    var controllerPipeName = getLocationServicePipeName();
    if (!controllerPipeName) {
        controllerPipeName = url;
    }
    return getUnixSocketDir(url)
        .then(function (socketDir) {
        var dirCheck = q.defer();
        fs.access(socketDir, function (err) { return dirCheck.resolve(err ? false : true); });
        return dirCheck.promise
            .then(function (exists) {
            if (exists) {
                // delete socket dir
                unixSocketDirCreated = false;
                return q.nfcall(rmdirRecursive, socketDir);
            }
        });
    });
}
exports.deleteUnixSocketDir = deleteUnixSocketDir;
function getPropertyNoCase(obj, propertyName) {
    if (!obj || !propertyName) {
        return null;
    }
    if (obj.hasOwnProperty(propertyName)) {
        return obj[propertyName];
    }
    propertyName = propertyName.toLowerCase();
    for (var prop in obj) {
        if (obj.hasOwnProperty(prop) && prop.toLowerCase() === propertyName) {
            return obj[prop];
        }
    }
}
exports.getPropertyNoCase = getPropertyNoCase;
function getPropertiesArray(obj) {
    var result = [];
    if (obj !== null && typeof obj === 'object') {
        for (var prop in obj) {
            if (obj.hasOwnProperty(prop)) {
                result.push(obj[prop]);
            }
        }
    }
    return result;
}
exports.getPropertiesArray = getPropertiesArray;
function getLogSessionKey() {
    var sessionKey = process.env[exports.UniqueLogSessionKeyEnvVarName];
    if (!sessionKey) {
        sessionKey = sha256(process.cwd().toLowerCase() + process.pid).toLowerCase().substring(0, 8);
        process.env[exports.UniqueLogSessionKeyEnvVarName] = sessionKey;
    }
    return sessionKey;
}
exports.getLogSessionKey = getLogSessionKey;
function getLogFilesDir() {
    if (process.platform == 'win32') {
        return path.join(os.tmpdir(), 'servicehub', 'logs');
    }
    else {
        return path.join(getServiceHubBaseDirForUnix(), 'logs');
    }
}
exports.getLogFilesDir = getLogFilesDir;
var locationServicePipeNameEnvironmentVariable = 'ServiceHubLocationServicePipeName';
function getLocationServicePipeName() {
    return process.env[locationServicePipeNameEnvironmentVariable];
}
exports.getLocationServicePipeName = getLocationServicePipeName;
function setLocationServicePipeName(pipeName) {
    process.env[locationServicePipeNameEnvironmentVariable] = pipeName || ''; // Process environment variables are always strings.
}
exports.setLocationServicePipeName = setLocationServicePipeName;
exports.maxPipeNameLength = 10;
function formatPipeName(pipeName) {
    if (process.platform === 'win32' || pipeName.length <= exports.maxPipeNameLength) {
        return pipeName;
    }
    else {
        return pipeName.substring(0, exports.maxPipeNameLength);
    }
}
exports.formatPipeName = formatPipeName;
function parseFileAsJson(filePath, encoding) {
    if (!filePath) {
        throw new ServiceHubError(ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variableIsNotDefined('filePath'));
    }
    encoding = encoding || 'utf8';
    return q.ninvoke(fs, 'readFile', filePath, encoding)
        .then(function (json) {
        try {
            // BOM is not a valid JSON token, remove it.
            // Node doesn't do that automatically, see https://github.com/nodejs/node-v0.x-archive/issues/1918
            return JSON.parse(json.replace(/^\uFEFF/, ''));
        }
        catch (err) {
            throw new ServiceHubError(ErrorKind.JsonParseError, libraryResourceStrings_1.LibraryResourceStrings.errorParsingJson(filePath, err.message));
        }
    });
}
exports.parseFileAsJson = parseFileAsJson;
// Private (non-exported) helper methods
var serviceHubBaseDirForUnix;
function getServiceHubBaseDirForUnix() {
    if (!serviceHubBaseDirForUnix) {
        serviceHubBaseDirForUnix = path.join(require('user-home'), '.ServiceHub');
    }
    return serviceHubBaseDirForUnix;
}


},{"./libraryResourceStrings":8,"./randomHexString":10,"errno-codes":13,"fs":undefined,"js-sha256":14,"mkdir-parents":15,"net":undefined,"os":undefined,"path":undefined,"q":18,"report-errors":25,"rmdir-recursive":28,"tmp":29,"user-home":30}],2:[function(require,module,exports){
"use strict";
var net = require('net');
var q_1 = require('q');
var common_1 = require('./common');
var libraryResourceStrings_1 = require('./libraryResourceStrings');
var ec = require('errno-codes');
var defaultTimeoutMs = 5000;
var defaultRetryIntervalMs = 100;
function connectWithRetries(options) {
    if (!options) {
        throw new common_1.ServiceHubError(common_1.ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variablesAreNotDefined('options'));
    }
    if (!options.url) {
        throw new common_1.ServiceHubError(common_1.ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variablesAreNotDefined('options.url'));
    }
    var noRetryAfterDateTime = new Date().getTime() + (options.timeoutMs || defaultTimeoutMs);
    var retryIntervalMs = options.retryIntervalMs || defaultRetryIntervalMs;
    return common_1.getServerPipePath(options.url)
        .then(function (serverPath) { return netConnectWithTimeout(serverPath, noRetryAfterDateTime, retryIntervalMs, options.cancellationToken); });
}
exports.connectWithRetries = connectWithRetries;
function connect(url) {
    if (!url) {
        throw new common_1.ServiceHubError(common_1.ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variablesAreNotDefined('url'));
    }
    return common_1.getServerPipePath(url)
        .then(function (serverPath) { return netConnect(serverPath); });
}
exports.connect = connect;
function netConnect(pipePath) {
    var result = q_1.defer();
    var stream = net.connect(pipePath);
    stream.once('connect', function () {
        result.resolve(stream);
    });
    stream.once('error', function (err) {
        result.reject(err);
    });
    return result.promise;
}
function netConnectWithTimeout(pipePath, noRetryAfterDateTime, retryIntervalMs, cancellationToken) {
    return netConnect(pipePath)
        .catch(function (err) {
        // We check for ec.ENOENT for the case where there is no controller running yet. We also need to check for ec.ECONNREFUSED for the case where
        // stale unix domain sockets files prevent clients from connecting. In both cases the controller will eventually come online and so we keep
        // trying to connect in both cases until timeout.
        var retryErrorCode = (err && (err.code === ec.ENOENT.code || err.code === ec.ECONNREFUSED.code));
        var cancellationNotRequested = (!cancellationToken || !cancellationToken.isCancellationRequested);
        if (retryErrorCode && cancellationNotRequested && new Date().getTime() < noRetryAfterDateTime) {
            return q_1.delay(retryIntervalMs)
                .then(function () { return netConnectWithTimeout(pipePath, noRetryAfterDateTime, retryIntervalMs, cancellationToken); });
        }
        throw err;
    });
}


},{"./common":1,"./libraryResourceStrings":8,"errno-codes":13,"net":undefined,"q":18}],3:[function(require,module,exports){
// Keep in sync with ..\CoreClr\src\Microsoft.ServiceHub.Client\ExitCode.cs
"use strict";
/**
 * Exit code from a Node ServiceHub process.
 * Range 1-19 and 128+ is used by Node itself.
 * Range 20 - 127 is used by ServiceHub.
 * Non-node ServiceHub hosts are supposed to use the same ServiceHub exit codes in ServiceHub range 20-127.
 * */
var ExitCode;
(function (ExitCode) {
    /** Successful termination, no error */
    ExitCode[ExitCode["Success"] = 0] = "Success";
    // Node error codes
    /** There was an uncaught exception, and it was not handled by a domain or an 'uncaughtException' event handler. */
    ExitCode[ExitCode["NodeUncaughtFatalException"] = 1] = "NodeUncaughtFatalException";
    /** The JavaScript source code internal in Node.js's bootstrapping process caused a parse error.
     * This is extremely rare, and generally can only happen during development of Node.js itself. */
    ExitCode[ExitCode["NodeInternalJavaScriptParseError"] = 3] = "NodeInternalJavaScriptParseError";
    /** The JavaScript source code internal in Node.js's bootstrapping process failed to return a function value when evaluated.
     * This is extremely rare, and generally can only happen during development of Node.js itself. */
    ExitCode[ExitCode["NodeInternalJavaScriptEvaluationFailure"] = 4] = "NodeInternalJavaScriptEvaluationFailure";
    /** There was a fatal unrecoverable error in V8. Typically a message will be printed to stderr with the prefix FATAL ERROR */
    ExitCode[ExitCode["NodeFatalError"] = 5] = "NodeFatalError";
    /** There was an uncaught exception, but the internal fatal exception handler function was somehow set to a non-function, and could not be called. */
    ExitCode[ExitCode["NodeNonFunctionInternalExceptionHandler"] = 6] = "NodeNonFunctionInternalExceptionHandler";
    /** There was an uncaught exception, and the internal fatal exception handler function itself threw an error while attempting to handle it.
     * This can happen, for example, if a process.on('uncaughtException') or domain.on('error') handler throws an error. */
    ExitCode[ExitCode["NodeInternalExceptionHandlerRunTimeFailure"] = 7] = "NodeInternalExceptionHandlerRunTimeFailure";
    /** Either an unknown option was specified, or an option requiring a value was provided without a value. */
    ExitCode[ExitCode["NodeInvalidArgument"] = 9] = "NodeInvalidArgument";
    /** The JavaScript source code internal in Node.js's bootstrapping process threw an error when the bootstrapping function was called.
     * This is extremely rare, and generally can only happen during development of Node.js itself. */
    ExitCode[ExitCode["NodeInternalJavaScriptRunTimeFailure"] = 10] = "NodeInternalJavaScriptRunTimeFailure";
    /** The --debug and/or --debug-brk options were set, but an invalid port number was chosen. */
    ExitCode[ExitCode["NodeInvalidDebugArgument"] = 12] = "NodeInvalidDebugArgument";
    // ServiceHub exit codes
    // -------------------------------------------------------------------------------------------------------
    /** Invalid command line argument for servicehub process */
    ExitCode[ExitCode["InvalidArgument"] = 20] = "InvalidArgument";
    /** Cannot start a new pipe server because the supplied pipe address is already in use.
     * This may happen when starting a controller where there is already a controller running */
    ExitCode[ExitCode["ErrorStartingServerPipeInUse"] = 21] = "ErrorStartingServerPipeInUse";
    /** Cannot start a new pipe server due to some error */
    ExitCode[ExitCode["ErrorStartingServer"] = 22] = "ErrorStartingServer";
    /** Uncaught exception */
    ExitCode[ExitCode["UncaughtException"] = 23] = "UncaughtException";
    /** Error shutting down */
    ExitCode[ExitCode["ShutdownError"] = 24] = "ShutdownError";
    /** Configuration error in servicehub.config.json */
    ExitCode[ExitCode["ConfigurationError"] = 25] = "ConfigurationError";
    // Signals
    // -------------------------------------------------------------------------------------------------------
    /** Node process was terminated by SIGHUP signal (when the console window is closed)*/
    ExitCode[ExitCode["SigHup"] = 129] = "SigHup";
    /** Node process was terminated by SIGINT signal (by pressing Ctrl-C in the console) */
    ExitCode[ExitCode["SigInt"] = 130] = "SigInt";
    /** Node process was terminated by SIGKILL signal */
    ExitCode[ExitCode["SigKill"] = 137] = "SigKill";
    /** Node  process was terminated by SIGTERM signal */
    ExitCode[ExitCode["SigTerm"] = 143] = "SigTerm";
    /** Node  process was terminated by SIGBREAK signal (by pressing Ctrl-Break in the console) */
    ExitCode[ExitCode["SigBreak"] = 149] = "SigBreak";
})(ExitCode || (ExitCode = {}));
Object.defineProperty(exports, "__esModule", { value: true });
exports.default = ExitCode;


},{}],4:[function(require,module,exports){
"use strict";
var __extends = (this && this.__extends) || function (d, b) {
    for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
    function __() { this.constructor = d; }
    d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
};
var q = require('q');
var e = require('events');
var sm = require('./serviceManager');
var c = require('./common');
var jsonRpc_1 = require('./jsonRpc');
var log = require('./logger');
var exitCode_1 = require('./exitCode');
var serviceModuleInfo_1 = require('./serviceModuleInfo');
var libraryResourceStrings_1 = require('./libraryResourceStrings');
var connect_1 = require('./connect');
var hostLogPrefix = 'hubHostNode';
var Host = (function (_super) {
    __extends(Host, _super);
    function Host(settings) {
        var _this = this;
        _super.call(this);
        this.pid = process.pid;
        if (!settings) {
            throw new c.ServiceHubError(c.ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variablesAreNotDefined('hostSettings'));
        }
        this.settings = settings;
        this.logger = settings.logger || new log.Logger(hostLogPrefix);
        this.logger.info(libraryResourceStrings_1.LibraryResourceStrings.startingHubHost(settings.hostId, settings.controllerPipeName));
        this.endPoint = connect_1.connect(settings.controllerPipeName)
            .then(function (stream) { return jsonRpc_1.JsonRpcConnection.attach(stream, _this.logger, _this, [
            'getHostId',
            'startService',
            'cancelServiceRequest',
            'exit',
        ]); });
        this.serviceManager = new sm.ServiceManager(this.logger);
        this.serviceManager.on(sm.ServiceManager.ServicesStartedEvent, this.onServiceManagerServicesStarted.bind(this));
        this.serviceManager.on(sm.ServiceManager.ServicesEndedEvent, this.onServiceManagerServicesEnded.bind(this));
    }
    Host.prototype.getHostId = function () {
        return this.settings.hostId;
    };
    Host.prototype.startService = function (info, serializedServiceActivationOptions) {
        var _this = this;
        if (this.shutdown) {
            var message = libraryResourceStrings_1.LibraryResourceStrings.startServiceRequestRejected(info.name);
            this.logger.error(message);
            return q.reject(new Error(message));
        }
        this.logger.info(libraryResourceStrings_1.LibraryResourceStrings.startServiceRequestReceived(info.name));
        var smi = new serviceModuleInfo_1.ServiceModuleInfo(info);
        return this.serviceManager.startService(smi)
            .then(function (serviceEndPoint) {
            _this.logger.info(libraryResourceStrings_1.LibraryResourceStrings.serviceStarted(smi.name, process.pid.toString(), serviceEndPoint));
            return serviceEndPoint;
        });
    };
    Host.prototype.cancelServiceRequest = function (pipeName) {
        c.serverManager.cancelServiceRequest(pipeName);
    };
    Host.prototype.exit = function () {
        var _this = this;
        if (this.shutdown) {
            this.logger.error(libraryResourceStrings_1.LibraryResourceStrings.ignoringDuplicateExit);
            return;
        }
        this.shutdown = true;
        this.logger.info(libraryResourceStrings_1.LibraryResourceStrings.exitCommandReceived);
        this.endPoint
            .then(function (e) {
            e.dispose();
            return c.serverManager.stop();
        })
            .done(function () { return _this.emitExit(exitCode_1.default.Success); }, function (reason) {
            _this.logger.error(libraryResourceStrings_1.LibraryResourceStrings.closeHubStreamFailed(reason.message || reason));
            _this.emitExit(exitCode_1.default.ShutdownError);
        });
    };
    Host.prototype.emitExit = function (exitCode) {
        this.emit('exit', exitCode);
    };
    Host.prototype.onServiceManagerServicesStarted = function (name, stream) {
        var _this = this;
        this.endPoint
            .then(function (e) {
            if (_this.shutdown) {
                _this.logger.error(libraryResourceStrings_1.LibraryResourceStrings.serviceStartedAfterHostShutDown(name));
                stream.end();
            }
            else {
                e.sendNotification('hostServicesStarted', [_this.settings.hostId]);
            }
        })
            .catch(function (reason) { return _this.logger.error(libraryResourceStrings_1.LibraryResourceStrings.notifyHubControllerForServicesStartedFailed(reason.message || reason)); })
            .done();
    };
    Host.prototype.onServiceManagerServicesEnded = function () {
        var _this = this;
        this.endPoint
            .then(function (e) {
            if (!_this.shutdown) {
                e.sendNotification('hostServicesEnded', [_this.settings.hostId]);
            }
        })
            .catch(function (reason) { return _this.logger.error(libraryResourceStrings_1.LibraryResourceStrings.notifyHubControllerForServicesStartedFailed(reason.message || reason)); })
            .done();
    };
    return Host;
}(e.EventEmitter));
exports.Host = Host;


},{"./common":1,"./connect":2,"./exitCode":3,"./jsonRpc":7,"./libraryResourceStrings":8,"./logger":9,"./serviceManager":11,"./serviceModuleInfo":12,"events":undefined,"q":18}],5:[function(require,module,exports){
"use strict";
var h = require('../host');
var c = require('../common');
var exitCode_1 = require('../exitCode');
var hostResourceStrings_1 = require('./hostResourceStrings');
c.registerForCrashReporting(/* crashFileLogPrefix */ 'NodeHost');
c.setOsLocale([hostResourceStrings_1.HostResourceStrings.config]);
if (process.argv.length < 4) {
    console.log(hostResourceStrings_1.HostResourceStrings.usage('node HubHost.js <HostId> <Controller Pipe Name>'));
    process.exit(exitCode_1.default.InvalidArgument);
}
var hostId = process.argv[2];
var controllerPipeName = process.argv[3];
var host = new h.Host({ hostId: hostId, controllerPipeName: controllerPipeName });
host.once('exit', function (code) { return process.exit(code); });


},{"../common":1,"../exitCode":3,"../host":4,"./hostResourceStrings":6}],6:[function(require,module,exports){
"use strict";
var nls = require('vscode-nls');
var path = require('path');
var defaultLocale = 'en';
// nls.config(...)() call below will be rewritten to nls.config(...)(__filename) in build time where __filename is supposed to '*ResourceStrings.js'
// so that nls can find *ResourceStrings.nls.json.
// Redefining __filename here because when using browserify, otherwise the __filename will point to browserified file instead of this file.
var __filename = path.join(__dirname, 'controllerResourceStrings.js');
var localize = nls.config({ locale: defaultLocale, cacheLanguageResolution: false })(__filename);
var HostResourceStrings = (function () {
    function HostResourceStrings() {
    }
    HostResourceStrings.config = function (locale, cache) {
        if (cache === void 0) { cache = false; }
        // nls.config(...)() call below will be rewritten to nls.config(...)(__filename) in build time where __filename is supposed to '*ResourceStrings.js'
        // so that nls can find *ResourceStrings.nls.json.
        // Redefining __filename here because when using browserify, otherwise the __filename will point to browserified file instead of this file.
        var __filename = path.join(__dirname, 'hostResourceStrings.js');
        localize = nls.config({ locale: locale, cacheLanguageResolution: cache })(__filename);
    };
    HostResourceStrings.usage = function (explaination) {
        return localize(0, null, explaination);
    };
    return HostResourceStrings;
}());
exports.HostResourceStrings = HostResourceStrings;

},{"path":undefined,"vscode-nls":38}],7:[function(require,module,exports){
"use strict";
var vscode_jsonrpc_1 = require('vscode-jsonrpc');
exports.Trace = vscode_jsonrpc_1.Trace;
exports.CancellationToken = vscode_jsonrpc_1.CancellationToken;
exports.CancellationTokenSource = vscode_jsonrpc_1.CancellationTokenSource;
var vscode = require('vscode-jsonrpc');
var q = require('q');
var q_1 = require('q');
var logger_1 = require('./logger');
var ec = require('errno-codes');
var JsonRpcConnection = (function () {
    function JsonRpcConnection(stream, logger) {
        this.stream = stream;
        this.logger = logger;
        this.requests = {};
        this.requestCount = 0;
        this.connection = vscode.createClientMessageConnection(stream, stream, logger);
        this.onClose = this.connection.onClose;
        this.connection.onClose(this.onConnectionClosed, this);
        this.onError = this.connection.onError;
        if (logger && logger.isEnabled(logger_1.LogLevel.Verbose)) {
            this.connection.trace(vscode_jsonrpc_1.Trace.Verbose, logger);
        }
    }
    JsonRpcConnection.attach = function (stream, logger, target, methodNames) {
        var connection = new JsonRpcConnection(stream, logger);
        if (target) {
            connection.onRequestInvokeTarget.apply(connection, [target].concat(methodNames));
        }
        connection.listen();
        return connection;
    };
    JsonRpcConnection.prototype.listen = function () {
        this.throwIfClosed();
        this.throwIfListening();
        this.connection.listen();
        this.listening = true;
    };
    JsonRpcConnection.prototype.sendRequest = function (method, params, token) {
        var _this = this;
        this.throwIfNotListening();
        if (this.closed) {
            return q.reject(JsonRpcConnection.createStreamClosedError(method));
        }
        var deferred = q_1.defer();
        var requestIndex = this.requestCount++;
        this.requests[requestIndex] = { method: method, deferred: deferred };
        this.connection
            .sendRequest({ 'method': method }, params, token)
            .then(function (value) {
            deferred.resolve(value);
            delete _this.requests[requestIndex];
        }, function (error) {
            deferred.reject(error);
            delete _this.requests[requestIndex];
        });
        return deferred.promise;
    };
    JsonRpcConnection.prototype.sendNotification = function (method, params) {
        if (this.closed) {
            return q.reject(JsonRpcConnection.createStreamClosedError(method));
        }
        this.connection.sendNotification({ 'method': method }, params);
        // Eventually we'll want to return a promise that is fulfilled after transmission has completed.
        return q(undefined);
    };
    JsonRpcConnection.prototype.onRequest = function (method, handler) {
        this.throwIfListening();
        this.throwIfClosed();
        this.connection.onRequest({ 'method': method }, function (args, ct) { return handler.apply(void 0, (args || []).concat([ct])); });
        this.connection.onNotification({ 'method': method }, function (args) { return handler.apply(void 0, args); });
    };
    JsonRpcConnection.prototype.onRequestInvokeTarget = function (target) {
        var _this = this;
        var methodNames = [];
        for (var _i = 1; _i < arguments.length; _i++) {
            methodNames[_i - 1] = arguments[_i];
        }
        this.throwIfListening();
        this.throwIfClosed();
        if (!methodNames || methodNames.length === 0) {
            methodNames = getMethodNamesOf(target);
        }
        methodNames.forEach(function (methodName) {
            var method = target[methodName];
            if (!method)
                throw new Error("Method not found " + methodName + ".");
            _this.onRequest(methodName, method.bind(target));
        });
    };
    JsonRpcConnection.prototype.dispose = function () {
        // Reject any pending outbound requests that haven't resolved already, since we won't get responses here on out.
        // TODO: code here, or get fix for https://github.com/Microsoft/vscode-languageserver-node/issues/65
        this.stream.end();
        this.connection.dispose();
    };
    JsonRpcConnection.prototype.cancelPendingRequests = function (reason) {
        for (var propName in this.requests) {
            if (this.requests.hasOwnProperty(propName)) {
                var request = this.requests[propName];
                request.deferred.reject(reason || JsonRpcConnection.createStreamClosedError(request.method));
            }
        }
        this.requests = {};
    };
    JsonRpcConnection.prototype.throwIfNotListening = function () {
        if (!this.listening) {
            throw new Error('Call listen() first.');
        }
    };
    JsonRpcConnection.prototype.throwIfListening = function () {
        if (this.listening) {
            throw new Error('listen() has already been called.');
        }
    };
    JsonRpcConnection.prototype.throwIfClosed = function (method) {
        if (this.closed) {
            throw JsonRpcConnection.createStreamClosedError(method);
        }
    };
    JsonRpcConnection.prototype.onConnectionClosed = function () {
        this.closed = true;
        this.cancelPendingRequests();
    };
    JsonRpcConnection.createStreamClosedError = function (method) {
        var message = (method ? "Cannot execute '" + method + "'. " : '') + 'The underlying stream has closed.';
        var result = new Error(message);
        result.code = ec.ENOENT.code;
        return result;
    };
    return JsonRpcConnection;
}());
exports.JsonRpcConnection = JsonRpcConnection;
function getMethodNamesOf(target) {
    var methods = [];
    for (var propertyName in target) {
        var propertyValue = target[propertyName];
        if (typeof propertyValue === 'function') {
            methods.push(propertyName);
        }
    }
    return methods;
}


},{"./logger":9,"errno-codes":13,"q":18,"vscode-jsonrpc":34}],8:[function(require,module,exports){
"use strict";
var c = require('./common');
var nls = require('vscode-nls');
var path = require('path');
var defaultLocale = 'en';
// nls.config(...)() call below will be rewritten to nls.config(...)(__filename) in build time where __filename is supposed to '*ResourceStrings.js'
// so that nls can find *ResourceStrings.nls.json.
// Redefining __filename here because when using browserify, otherwise the __filename will point to browserified file instead of this file.
var __filename = path.join(__dirname, 'libraryResourceStrings.js');
var localize = nls.config({ locale: defaultLocale, cacheLanguageResolution: false })(__filename);
var LibraryResourceStrings = (function () {
    function LibraryResourceStrings() {
    }
    LibraryResourceStrings.init = function () {
        if (!LibraryResourceStrings.isOsLocaleSet) {
            c.setOsLocale([LibraryResourceStrings.config]);
            LibraryResourceStrings.isOsLocaleSet = true;
        }
    };
    LibraryResourceStrings.config = function (locale, cache) {
        if (cache === void 0) { cache = false; }
        // nls.config(...)() call below will be rewritten to nls.config(...)(__filename) in build time where __filename is supposed to '*ResourceStrings.js'
        // so that nls can find *ResourceStrings.nls.json.
        // Redefining __filename here because when using browserify, otherwise the __filename will point to browserified file instead of this file.
        var __filename = path.join(__dirname, 'libraryResourceStrings.js');
        localize = nls.config({ locale: locale, cacheLanguageResolution: cache })(__filename);
    };
    LibraryResourceStrings.variablesAreNotDefined = function (variable) {
        LibraryResourceStrings.init();
        return localize(0, null, variable);
    };
    LibraryResourceStrings.variableIsNotDefined = function (variable) {
        LibraryResourceStrings.init();
        return localize(1, null, variable);
    };
    LibraryResourceStrings.startingServer = function (name, url, serverPath) {
        LibraryResourceStrings.init();
        return localize(2, null, name, url, serverPath);
    };
    LibraryResourceStrings.stoppedServer = function (name, url) {
        LibraryResourceStrings.init();
        return localize(3, null, name, url);
    };
    LibraryResourceStrings.urlShouldStartWith = function (scheme) {
        LibraryResourceStrings.init();
        return localize(4, null, scheme);
    };
    LibraryResourceStrings.urlHasNoPipeName = function (scheme) {
        LibraryResourceStrings.init();
        return localize(5, null, scheme);
    };
    LibraryResourceStrings.noPipeName = function (scheme) {
        LibraryResourceStrings.init();
        return localize(6, null, scheme);
    };
    LibraryResourceStrings.errorParsingJson = function (filePath, err) {
        LibraryResourceStrings.init();
        return localize(7, null, filePath, err);
    };
    LibraryResourceStrings.serviceHubConfigJsonNotFound = function (fileName, dir) {
        LibraryResourceStrings.init();
        return localize(8, null, fileName, dir);
    };
    LibraryResourceStrings.loading = function (path) {
        LibraryResourceStrings.init();
        return localize(9, null, path);
    };
    LibraryResourceStrings.noValueForProperty = function (fileName, dir, property) {
        LibraryResourceStrings.init();
        return localize(10, null, fileName, dir, property);
    };
    Object.defineProperty(LibraryResourceStrings, "failedGetSaltString", {
        get: function () {
            LibraryResourceStrings.init();
            return localize(11, null);
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(LibraryResourceStrings, "noLocalAppDataFolder", {
        get: function () {
            LibraryResourceStrings.init();
            return localize(12, null);
        },
        enumerable: true,
        configurable: true
    });
    LibraryResourceStrings.multipleFileFound = function (dir, files) {
        LibraryResourceStrings.init();
        return localize(13, null, dir, files);
    };
    LibraryResourceStrings.startingHubHost = function (hostId, pipeName) {
        LibraryResourceStrings.init();
        return localize(14, null, hostId, pipeName);
    };
    LibraryResourceStrings.startServiceRequestRejected = function (serviceName) {
        LibraryResourceStrings.init();
        return localize(15, null, serviceName);
    };
    LibraryResourceStrings.startServiceRequestReceived = function (serviceName) {
        LibraryResourceStrings.init();
        return localize(16, null, serviceName);
    };
    LibraryResourceStrings.serviceStarted = function (serviceName, pid, endPoint) {
        LibraryResourceStrings.init();
        return localize(17, null, serviceName, pid, endPoint);
    };
    Object.defineProperty(LibraryResourceStrings, "ignoringDuplicateExit", {
        get: function () {
            LibraryResourceStrings.init();
            return localize(18, null);
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(LibraryResourceStrings, "exitCommandReceived", {
        get: function () {
            LibraryResourceStrings.init();
            return localize(19, null);
        },
        enumerable: true,
        configurable: true
    });
    LibraryResourceStrings.closeHubStreamFailed = function (error) {
        LibraryResourceStrings.init();
        return localize(20, null, error);
    };
    LibraryResourceStrings.serviceStartedAfterHostShutDown = function (serviceName) {
        LibraryResourceStrings.init();
        return localize(21, null, serviceName);
    };
    LibraryResourceStrings.notifyHubControllerForServicesStartedFailed = function (error) {
        LibraryResourceStrings.init();
        return localize(22, null, error);
    };
    LibraryResourceStrings.notifyHubControllerForServicesEndedFailed = function (error) {
        LibraryResourceStrings.init();
        return localize(23, null, error);
    };
    Object.defineProperty(LibraryResourceStrings, "serviceNameNotSpecified", {
        get: function () {
            LibraryResourceStrings.init();
            return localize(24, null);
        },
        enumerable: true,
        configurable: true
    });
    LibraryResourceStrings.locatingService = function (serviceName, clientId) {
        LibraryResourceStrings.init();
        return localize(25, null, serviceName, clientId);
    };
    LibraryResourceStrings.serviceLocated = function (serviceId, serviceLocation, clientId) {
        LibraryResourceStrings.init();
        return localize(26, null, serviceId, serviceLocation, clientId);
    };
    LibraryResourceStrings.startingServiceFailed = function (serviceId, clientId, error) {
        LibraryResourceStrings.init();
        return localize(27, null, serviceId, clientId, error);
    };
    Object.defineProperty(LibraryResourceStrings, "forceControllerShutDown", {
        get: function () {
            LibraryResourceStrings.init();
            return localize(28, null);
        },
        enumerable: true,
        configurable: true
    });
    LibraryResourceStrings.locatingServcieError = function (discoveryService, service, error, callStack) {
        LibraryResourceStrings.init();
        return localize(29, null, discoveryService, service, error, callStack);
    };
    LibraryResourceStrings.disconnectingDiscoveryServiceFailed = function (discoveryService, error, callStack) {
        LibraryResourceStrings.init();
        return localize(30, null, discoveryService, error, callStack);
    };
    LibraryResourceStrings.hostTypeNotFoundInLocation = function (hostType, location, additionalLocation) {
        LibraryResourceStrings.init();
        return localize(31, null, hostType, location, additionalLocation);
    };
    LibraryResourceStrings.hostTypeNotFound = function (hostType, location) {
        LibraryResourceStrings.init();
        return localize(32, null, hostType, location);
    };
    LibraryResourceStrings.loadingHostInfo = function (host, filePath) {
        LibraryResourceStrings.init();
        return localize(33, null, host, filePath);
    };
    LibraryResourceStrings.cannotResolveHost = function (host, filePath, error) {
        LibraryResourceStrings.init();
        return localize(34, null, host, filePath, error);
    };
    LibraryResourceStrings.hostInfoNotDefined = function (path) {
        LibraryResourceStrings.init();
        return localize(35, null, path);
    };
    LibraryResourceStrings.hostInfoInvalid = function (path) {
        LibraryResourceStrings.init();
        return localize(36, null, path);
    };
    LibraryResourceStrings.hostInfoContainsReservedProperty = function (path) {
        LibraryResourceStrings.init();
        return localize(37, null, path);
    };
    LibraryResourceStrings.firstTimeLoadingService = function (serviceName) {
        LibraryResourceStrings.init();
        return localize(38, null, serviceName);
    };
    LibraryResourceStrings.launchingHostWithCmd = function (host, cmd) {
        LibraryResourceStrings.init();
        return localize(39, null, host, cmd);
    };
    LibraryResourceStrings.hostLaunched = function (host, pid) {
        LibraryResourceStrings.init();
        return localize(40, null, host, pid);
    };
    LibraryResourceStrings.hostExited = function (host, pid) {
        LibraryResourceStrings.init();
        return localize(41, null, host, pid);
    };
    LibraryResourceStrings.hostExitedWithCode = function (code, exitCode) {
        LibraryResourceStrings.init();
        return localize(42, null, code, exitCode);
    };
    LibraryResourceStrings.hostExitedWithSignal = function (signal) {
        LibraryResourceStrings.init();
        return localize(43, null, signal);
    };
    LibraryResourceStrings.startingHostError = function (executable, hostId, error) {
        LibraryResourceStrings.init();
        return localize(44, null, executable, hostId, error);
    };
    LibraryResourceStrings.hostInfoNotFound = function (id) {
        LibraryResourceStrings.init();
        return localize(45, null, id);
    };
    LibraryResourceStrings.getHostIdFailed = function (error) {
        LibraryResourceStrings.init();
        return localize(46, null, error);
    };
    Object.defineProperty(LibraryResourceStrings, "moreThanOneHostPending", {
        get: function () {
            LibraryResourceStrings.init();
            return localize(47, null);
        },
        enumerable: true,
        configurable: true
    });
    LibraryResourceStrings.unknownHost = function (hostId) {
        LibraryResourceStrings.init();
        return localize(48, null, hostId);
    };
    LibraryResourceStrings.stoppingHost = function (hostId, pid, reason) {
        LibraryResourceStrings.init();
        return localize(49, null, hostId, pid, reason);
    };
    Object.defineProperty(LibraryResourceStrings, "cannotStopLocationService", {
        get: function () {
            LibraryResourceStrings.init();
            return localize(50, null);
        },
        enumerable: true,
        configurable: true
    });
    LibraryResourceStrings.errorDecoding = function (error) {
        LibraryResourceStrings.init();
        return localize(51, null, error);
    };
    LibraryResourceStrings.logConfig = function (config) {
        LibraryResourceStrings.init();
        return localize(52, null, config);
    };
    Object.defineProperty(LibraryResourceStrings, "messageAt", {
        get: function () {
            LibraryResourceStrings.init();
            return localize(53, null);
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(LibraryResourceStrings, "lengthCannotBeNegative", {
        get: function () {
            LibraryResourceStrings.init();
            return localize(54, null);
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(LibraryResourceStrings, "noFolderSpecified", {
        get: function () {
            LibraryResourceStrings.init();
            return localize(55, null);
        },
        enumerable: true,
        configurable: true
    });
    LibraryResourceStrings.cannotFindServiceModuleFile = function (filePattern, folder) {
        LibraryResourceStrings.init();
        return localize(56, null, filePattern, folder);
    };
    LibraryResourceStrings.cannotFindServiceModuleFileWithReason = function (filePattern, folder, error) {
        LibraryResourceStrings.init();
        return localize(57, null, filePattern, folder, error);
    };
    LibraryResourceStrings.loadingServiceModule = function (filePath, service) {
        LibraryResourceStrings.init();
        return localize(58, null, filePath, service);
    };
    LibraryResourceStrings.serviceInfoInvalid = function (filePath, service, property) {
        LibraryResourceStrings.init();
        return localize(59, null, filePath, service, property);
    };
    LibraryResourceStrings.serviceInfoInvalidSpaceInHostId = function (filePath, service, hostId) {
        LibraryResourceStrings.init();
        return localize(60, null, filePath, service, hostId);
    };
    LibraryResourceStrings.cannotResolveServiceModuleInfo = function (filePath, service, error) {
        LibraryResourceStrings.init();
        return localize(61, null, service, filePath, error);
    };
    LibraryResourceStrings.startingService = function (service) {
        LibraryResourceStrings.init();
        return localize(62, null, service);
    };
    LibraryResourceStrings.variableMustBeFunction = function (variable, moduleName, actualType) {
        LibraryResourceStrings.init();
        return localize(63, null, variable, moduleName, actualType);
    };
    LibraryResourceStrings.serviceEntryPointFileNameNotFound = function (name) {
        LibraryResourceStrings.init();
        return localize(64, null, name);
    };
    LibraryResourceStrings.serviceEntryPointConstructorNotFound = function (name) {
        LibraryResourceStrings.init();
        return localize(65, null, name);
    };
    LibraryResourceStrings.loadServiceModuleError = function (moduleName, error) {
        LibraryResourceStrings.init();
        return localize(66, null, moduleName, error);
    };
    LibraryResourceStrings.connectedToService = function (name) {
        LibraryResourceStrings.init();
        return localize(67, null, name);
    };
    LibraryResourceStrings.disposeServiceError = function (instanceName, name, error) {
        LibraryResourceStrings.init();
        return localize(68, null, instanceName, name, error);
    };
    LibraryResourceStrings.createServiceInstanceError = function (name, error) {
        LibraryResourceStrings.init();
        return localize(69, null, name, error);
    };
    LibraryResourceStrings.failedToDeleteFile = function (path, errCode, errMessage) {
        LibraryResourceStrings.init();
        return localize(70, null, path, errCode, errMessage);
    };
    LibraryResourceStrings.failedToGetFileStats = function (path, errCode, errMessage) {
        LibraryResourceStrings.init();
        return localize(71, null, path, errCode, errMessage);
    };
    LibraryResourceStrings.failedToEnumerateFilesInDir = function (path, errCode, errMessage) {
        LibraryResourceStrings.init();
        return localize(72, null, path, errCode, errMessage);
    };
    LibraryResourceStrings.serviceHubConfigFilePathIsNotAbsolute = function (serviceHubConfigFilePath) {
        LibraryResourceStrings.init();
        return localize(73, null, serviceHubConfigFilePath);
    };
    LibraryResourceStrings.serviceHubConfigFileNameIsIncorrect = function (serviceHubConfigFilePath) {
        LibraryResourceStrings.init();
        return localize(74, null, serviceHubConfigFilePath);
    };
    LibraryResourceStrings.serviceHubConfigFileDoesNotExist = function (serviceHubConfigFilePath) {
        LibraryResourceStrings.init();
        return localize(75, null, serviceHubConfigFilePath);
    };
    LibraryResourceStrings.HostGroupNotSupported = function (serviceName, hostGroup, propertyName) {
        LibraryResourceStrings.init();
        return localize(76, null, serviceName, hostGroup, propertyName);
    };
    Object.defineProperty(LibraryResourceStrings, "objectDisposed", {
        get: function () {
            LibraryResourceStrings.init();
            return localize(77, null);
        },
        enumerable: true,
        configurable: true
    });
    LibraryResourceStrings.couldNotKillHost = function (host, error) {
        LibraryResourceStrings.init();
        return localize(78, null, error);
    };
    return LibraryResourceStrings;
}());
exports.LibraryResourceStrings = LibraryResourceStrings;

},{"./common":1,"path":undefined,"vscode-nls":38}],9:[function(require,module,exports){
"use strict";
var fs = require('fs');
var path = require('path');
var os = require('os');
var util = require('util');
var c = require('./common');
var mkdirp = require('mkdir-parents');
var ec = require('errno-codes');
var tmp = require('tmp');
var exitCode_1 = require('./exitCode');
var libraryResourceStrings_1 = require('./libraryResourceStrings');
/**
 * Log levels
 */
(function (LogLevel) {
    LogLevel[LogLevel["Critical"] = 1] = "Critical";
    LogLevel[LogLevel["Error"] = 2] = "Error";
    LogLevel[LogLevel["Warning"] = 4] = "Warning";
    LogLevel[LogLevel["Information"] = 8] = "Information";
    LogLevel[LogLevel["Verbose"] = 16] = "Verbose";
})(exports.LogLevel || (exports.LogLevel = {}));
var LogLevel = exports.LogLevel;
exports.logFileDirectory = c.getLogFilesDir();
function traceProcessOutput(logger, process, processName) {
    if (!logger) {
        throw new c.ServiceHubError(c.ErrorKind.InvalidArgument, 'logger is not defined');
    }
    if (!process) {
        throw new c.ServiceHubError(c.ErrorKind.InvalidArgument, 'process is not defined');
    }
    if (!processName) {
        throw new c.ServiceHubError(c.ErrorKind.InvalidArgument, 'processName is not defined');
    }
    process.once('exit', function (code, signal) {
        var codeOrSignal = '';
        if (typeof code === 'number') {
            codeOrSignal = " with code " + code;
            if (exitCode_1.default[code]) {
                codeOrSignal += " (" + exitCode_1.default[code] + ")";
            }
        }
        if (signal) {
            codeOrSignal += " with signal '" + signal + "''";
        }
        logger.info("" + processName + getPid(process) + " exited" + codeOrSignal + ".");
    });
    process.once('error', function (err) {
        logger.error("" + processName + getPid(process) + " error " + err.message + ".");
    });
    process.stderr.on('data', function (data) {
        logger.error("" + processName + getPid(process) + " stderr: " + data.toString());
    });
    if (logger.isEnabled(LogLevel.Verbose)) {
        process.stdout.on('data', function (data) {
            logger.verbose("" + processName + getPid(process) + " stdout: " + data.toString());
        });
    }
}
exports.traceProcessOutput = traceProcessOutput;
function getPid(process) {
    return process.pid ? ' PID ' + process.pid : '';
}
/** Trace level, must be in sync with System.Diagnostic.SourceLevels */
var SourceLevels;
(function (SourceLevels) {
    /** Allows all events through. */
    SourceLevels[SourceLevels["All"] = -1] = "All";
    /** Does not allow any events through. */
    SourceLevels[SourceLevels["Off"] = 0] = "Off";
    /** Allows only Critical events through. */
    SourceLevels[SourceLevels["Critical"] = LogLevel.Critical] = "Critical";
    /** Allows Critical and Error events through. */
    SourceLevels[SourceLevels["Error"] = 3] = "Error";
    /** Allows Critical, Error, and Warning events through. */
    SourceLevels[SourceLevels["Warning"] = 7] = "Warning";
    /** Allows Critical, Error, Warning, and Information events through. */
    SourceLevels[SourceLevels["Information"] = 15] = "Information";
    /** Allows Critical, Error, Warning, Information, and Verbose events through. */
    SourceLevels[SourceLevels["Verbose"] = 31] = "Verbose";
})(SourceLevels || (SourceLevels = {}));
/** Trace level. Must be in sync with EnvUtils.TraceLevelEnvVarName in src\clr\utility\Microsoft.ServiceHub.Utility.Shared\EnvUtils.cs */
exports.SourceLevelEnvironmentVariable = 'SERVICEHUBTRACELEVEL';
function cleanUpOldLogs(delayInMsecBeforeStartingLogCleanup, deleteLogFilesOlderThanDate) {
    return setTimeout(function () {
        fs.exists(exports.logFileDirectory, function (exists) {
            if (!exists) {
                return;
            }
            var logger = new Logger('oldLogsCleaner');
            // get list of log files
            fs.readdir(exports.logFileDirectory, function (err, files) {
                if (err) {
                    logger.error(libraryResourceStrings_1.LibraryResourceStrings.failedToEnumerateFilesInDir(exports.logFileDirectory, err.code, err.message));
                    return;
                }
                for (var i = 0; i < files.length; i++) {
                    // get "Change Time" for each log file
                    (function (file) {
                        fs.stat(file, function (err, stats) {
                            if (err) {
                                logger.error(libraryResourceStrings_1.LibraryResourceStrings.failedToGetFileStats(file, err.code, err.message));
                                return;
                            }
                            // check if the log file is old enough to be deleted based on its "Change Time"
                            if (stats.ctime.getTime() < deleteLogFilesOlderThanDate.getTime()) {
                                // delete file as it hasn't been used since the specified number of days
                                fs.unlink(file, function (err) {
                                    if (err) {
                                        logger.error(libraryResourceStrings_1.LibraryResourceStrings.failedToDeleteFile(file, err.code, err.message));
                                    }
                                });
                            }
                        });
                    })(path.join(exports.logFileDirectory, files[i]));
                }
            });
        });
    }, delayInMsecBeforeStartingLogCleanup);
}
exports.cleanUpOldLogs = cleanUpOldLogs;
var Logger = (function () {
    function Logger(logFilePrefix, options) {
        this.maxFileSizeInBytes = 500 * 1024; // 500 KB
        this.nextLogFileNumber = 1;
        this.currentFileSizeInBytes = 0;
        this.encoding = 'utf8';
        if (!logFilePrefix) {
            throw new Error(libraryResourceStrings_1.LibraryResourceStrings.variableIsNotDefined('LogFilePrefix'));
        }
        options = options || {};
        this.defaultLevel = options.defaultLevel || LogLevel.Verbose;
        var sourceLevelsEnvironment = options.logSourceLevel || process.env[exports.SourceLevelEnvironmentVariable];
        this.sourceLevels = parseInt(sourceLevelsEnvironment);
        if (isNaN(this.sourceLevels)) {
            var enumValue = c.getPropertyNoCase(SourceLevels, sourceLevelsEnvironment);
            this.sourceLevels = typeof enumValue === 'number' ? enumValue : SourceLevels.Error;
        }
        var dir = options.logFileDirectory || exports.logFileDirectory;
        this.logFilePath = path.join(dir, tmp.tmpNameSync({ template: logFilePrefix + "-" + process.pid + "-XXXXXX", postfix: '.log' }) + '.log');
        this.logFilePrefix = tmp.tmpNameSync({ template: logFilePrefix + "-" + process.pid + "-XXXXXX", postfix: '.log' });
        this.logFilePath = this.getLogFilePath();
        this.rollingEnabled = !this.isEnabled(LogLevel.Verbose);
    }
    Logger.prototype.critical = function (message) {
        this.LogInternal(LogLevel.Critical, message);
    };
    Logger.prototype.error = function (message) {
        this.LogInternal(LogLevel.Error, message);
    };
    Logger.prototype.warn = function (message) {
        this.LogInternal(LogLevel.Warning, message);
    };
    Logger.prototype.info = function (message) {
        this.LogInternal(LogLevel.Information, message);
    };
    Logger.prototype.verbose = function (message) {
        this.LogInternal(LogLevel.Verbose, message);
    };
    Logger.prototype.log = function (message) {
        this.LogInternal(this.defaultLevel, message);
    };
    Logger.prototype.isEnabled = function (level) {
        return (this.sourceLevels & level) != 0;
    };
    Logger.prototype.LogInternal = function (level, message) {
        try {
            if (this.isEnabled(level)) {
                if (!this.started) {
                    this.started = true;
                    this.startLogging();
                }
                this.logMessage(level, message);
            }
        }
        catch (e) {
        }
    };
    Logger.prototype.startLogging = function () {
        var logDirectory = path.dirname(this.logFilePath);
        if (!fs.existsSync(logDirectory)) {
            try {
                mkdirp.sync(logDirectory, c.unixOwnerOnlyAccessMode);
            }
            catch (e) {
                if (e.code !== ec.EEXIST.code) {
                    throw e;
                }
            }
        }
        this.logMessage(LogLevel.Information, libraryResourceStrings_1.LibraryResourceStrings.logConfig("$" + exports.SourceLevelEnvironmentVariable + "=\"" + (process.env[exports.SourceLevelEnvironmentVariable] || '') + "\""));
        // don't count this message for our roll over file size calculation
        this.currentFileSizeInBytes = 0;
    };
    Logger.prototype.logMessage = function (level, message) {
        if (!message) {
            return;
        }
        var text;
        if (typeof message === 'string') {
            text = message;
        }
        else if (message.message) {
            text = message;
            if (message.stack) {
                text += os.EOL + libraryResourceStrings_1.LibraryResourceStrings.messageAt + ("" + os.EOL + message.stack);
            }
        }
        else {
            text = util.inspect(message);
        }
        var finalMessage = formatMessage(text, level);
        // Since message header and footer are short and of constant length managed ServiceHub logger ignores them to simplify file size calculation.
        // To be consistent we do the same here for JS logger. This is ok because we don't need an accurate file size for rolling over to a new file.
        // This also helps in keeping the tests simple.
        var buffer = new Buffer(text, this.encoding);
        if (this.rollingEnabled && this.currentFileSizeInBytes > 0 && (this.currentFileSizeInBytes + buffer.length > this.maxFileSizeInBytes)) {
            // delete lastLogFilePath
            if (this.lastLogFilePath) {
                fs.unlink(this.lastLogFilePath, function (err) { });
            }
            this.lastLogFilePath = this.logFilePath;
            this.logFilePath = this.getLogFilePath();
            fs.appendFileSync(this.logFilePath, formatMessage(libraryResourceStrings_1.LibraryResourceStrings.logConfig("$" + exports.SourceLevelEnvironmentVariable + "=\"" + (process.env[exports.SourceLevelEnvironmentVariable] || '') + "\""), LogLevel.Information), { encoding: this.encoding });
            this.currentFileSizeInBytes = 0;
        }
        fs.appendFileSync(this.logFilePath, finalMessage, { encoding: this.encoding });
        this.currentFileSizeInBytes += buffer.length;
    };
    Logger.prototype.getLogFilePath = function () {
        var sessionKey = c.getLogSessionKey();
        return path.join(exports.logFileDirectory, sessionKey + "-" + this.logFilePrefix + "-" + process.pid + "-" + this.nextLogFileNumber++ + ".log");
    };
    return Logger;
}());
exports.Logger = Logger;
function formatMessage(message, level) {
    return formatDateTime() + " : " + LogLevel[level] + " : " + message + os.EOL;
}
function twoDigits(v) {
    return "" + (v < 10 ? '0' : '') + v;
}
function formatTime(date) {
    return twoDigits(date.getHours()) + ":" + twoDigits(date.getMinutes()) + ":" + twoDigits(date.getSeconds());
}
function formatDateTime() {
    var now = new Date();
    return twoDigits(now.getMonth() + 1) + "/" + twoDigits(now.getDate()) + "/" + now.getFullYear() + " " + formatTime(now);
}


},{"./common":1,"./exitCode":3,"./libraryResourceStrings":8,"errno-codes":13,"fs":undefined,"mkdir-parents":15,"os":undefined,"path":undefined,"tmp":29,"util":undefined}],10:[function(require,module,exports){
"use strict";
var crypto_1 = require('crypto');
var q_1 = require('q');
var libraryResourceStrings_1 = require('./libraryResourceStrings');
exports.defaultRandomHexStringLength = 32;
function randomHexString(length, useCrypto) {
    if (length < 0) {
        throw new Error(libraryResourceStrings_1.LibraryResourceStrings.lengthCannotBeNegative);
    }
    length = length || exports.defaultRandomHexStringLength;
    if (useCrypto) {
        return q_1.nfcall(crypto_1.randomBytes, Math.round(length / 2))
            .then(function (buffer) { return buffer.toString('hex').substr(0, length); });
    }
    else {
        return q_1.resolve(getRandomHexString(length));
    }
}
Object.defineProperty(exports, "__esModule", { value: true });
exports.default = randomHexString;
function randomHexStringSync(length, useCrypto) {
    if (length < 0) {
        throw new Error(libraryResourceStrings_1.LibraryResourceStrings.lengthCannotBeNegative);
    }
    length = length || exports.defaultRandomHexStringLength;
    if (useCrypto) {
        return crypto_1.randomBytes(Math.round(length / 2)).toString('hex').substr(0, length);
    }
    else {
        return getRandomHexString(length);
    }
}
exports.randomHexStringSync = randomHexStringSync;
function getRandomHexString(length) {
    var random = ((1 + Math.random()) * 0x10000000000000).toString(16);
    while (random.length < length) {
        random = "" + random + ((1 + Math.random()) * 0x10000000000000).toString(16);
    }
    return (random.substr(0, length));
}


},{"./libraryResourceStrings":8,"crypto":undefined,"q":18}],11:[function(require,module,exports){
"use strict";
var __extends = (this && this.__extends) || function (d, b) {
    for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
    function __() { this.constructor = d; }
    d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
};
var path = require('path');
var fs = require('fs');
var e = require('events');
var q = require('q');
var c = require('./common');
var libraryResourceStrings_1 = require('./libraryResourceStrings');
/**
 * Service Manager
 *
 * Events:
 *     servicesStarted(serviceName, stream) - at least one client has open connection to a service running by this service manager.
 *     servicesEnded - there are no more open connections to any service running by this service manager.
 */
var ServiceManager = (function (_super) {
    __extends(ServiceManager, _super);
    function ServiceManager(logger) {
        _super.call(this);
        this.endPoints = [];
        this.streams = [];
        this.logger = logger;
    }
    ServiceManager.prototype.startService = function (serviceModuleInfo) {
        if (serviceModuleInfo == null) {
            throw new c.ServiceHubError(c.ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variableIsNotDefined('serviceModuleInfo'));
        }
        this.logger.info(libraryResourceStrings_1.LibraryResourceStrings.startingService(serviceModuleInfo.name));
        return this.startServiceObject(serviceModuleInfo);
    };
    ServiceManager.loadFunction = function (moduleName, constructor) {
        if (!moduleName) {
            throw new c.ServiceHubError(c.ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variableIsNotDefined('moduleName'));
        }
        if (!constructor) {
            throw new c.ServiceHubError(c.ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variableIsNotDefined('constructor'));
        }
        var checkFile = path.isAbsolute(moduleName) ? q.ninvoke(fs, 'access', moduleName, fs.F_OK | fs.R_OK) : q.resolve(null);
        return checkFile.then(function () {
            var nodeModule = require(moduleName);
            var nodeModuleExport = nodeModule[constructor];
            if (typeof nodeModuleExport !== 'function') {
                var message = libraryResourceStrings_1.LibraryResourceStrings.variableMustBeFunction(constructor, moduleName, typeof nodeModuleExport);
                throw new c.ServiceHubError(c.ErrorKind.InvalidOperation, message);
            }
            return function () {
                var params = [];
                for (var _i = 0; _i < arguments.length; _i++) {
                    params[_i - 0] = arguments[_i];
                }
                var result = Object.create(nodeModuleExport.prototype);
                result.constructor.apply(result, params);
                return result;
            };
        });
    };
    ServiceManager.prototype.startServiceObject = function (serviceModuleInfo) {
        var _this = this;
        var name = serviceModuleInfo.name;
        var serviceEntryPoint = (serviceModuleInfo.entryPoint);
        if (!serviceEntryPoint.scriptPath) {
            var message = libraryResourceStrings_1.LibraryResourceStrings.serviceEntryPointFileNameNotFound(name);
            this.logger.error(message);
            throw new c.ServiceHubError(c.ErrorKind.InvalidArgument, message);
        }
        if (!serviceEntryPoint.constructorFunction) {
            var message = libraryResourceStrings_1.LibraryResourceStrings.serviceEntryPointConstructorNotFound(name);
            this.logger.error(message);
            throw new c.ServiceHubError(c.ErrorKind.InvalidArgument, message);
        }
        var moduleName = ServiceManager.getFullPath(serviceModuleInfo, serviceEntryPoint.scriptPath);
        var constructor = serviceEntryPoint.constructorFunction;
        var serviceOptions = {
            name: name,
            logger: this.logger,
            noStreamTrace: true,
        };
        return ServiceManager.loadFunction(moduleName, constructor)
            .catch(function (err) {
            var message = libraryResourceStrings_1.LibraryResourceStrings.loadServiceModuleError(name, err.message);
            _this.logger.error(message);
            return q.reject(new Error(message));
        })
            .then(function (serviceModuleFactory) { return c.serverManager.startService(serviceOptions, function (stream, logger) {
            logger.info(libraryResourceStrings_1.LibraryResourceStrings.connectedToService(name));
            // Keep a reference to the service stream so it is not garbage collected.
            if (_this.streams.push(stream) === 1) {
                _this.emit(ServiceManager.ServicesStartedEvent, name, stream);
            }
            var serviceModuleInstance = null;
            stream.once('end', function () {
                logger.info("A stream to " + name + " ended.");
                var disposal = q.resolve(null);
                if (serviceModuleInstance && typeof serviceModuleInstance.disposeAsync === 'function') {
                    var instanceName_1 = serviceModuleInstance.constructor.name ? serviceModuleInstance.constructor.name + ' ' : '';
                    disposal = disposal
                        .then(function () { return serviceModuleInstance.disposeAsync(); })
                        .catch(function (reason) {
                        logger.error(libraryResourceStrings_1.LibraryResourceStrings.disposeServiceError(instanceName_1, name, reason.stack || reason.message || reason));
                        return null;
                    });
                }
                disposal.finally(function () {
                    var index = _this.streams.indexOf(stream);
                    if (index >= 0) {
                        _this.streams.splice(index, 1);
                    }
                    if (_this.streams.length === 0) {
                        _this.emit(ServiceManager.ServicesEndedEvent);
                    }
                }).done();
            });
            try {
                var svcs = {
                    logger: _this.logger,
                };
                serviceModuleInstance = serviceModuleFactory.call(_this, stream, svcs);
            }
            catch (err) {
                logger.error(libraryResourceStrings_1.LibraryResourceStrings.createServiceInstanceError(name, err.stack || err.message || err));
                stream.end();
                return;
            }
        }); });
    };
    ServiceManager.getFullPath = function (smi, fileNameOrPath) {
        if (!fileNameOrPath) {
            throw new c.ServiceHubError(c.ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variableIsNotDefined('fileNameOrPath'));
        }
        var result;
        if (path.isAbsolute(fileNameOrPath)) {
            result = fileNameOrPath;
        }
        else {
            result = smi.serviceBaseDirectory ? path.join(smi.serviceBaseDirectory, fileNameOrPath) : fileNameOrPath;
        }
        return result.replace(/\\/g, '/');
    };
    ServiceManager.ServicesStartedEvent = 'servicesStarted';
    ServiceManager.ServicesEndedEvent = 'servicesEnded';
    return ServiceManager;
}(e.EventEmitter));
exports.ServiceManager = ServiceManager;


},{"./common":1,"./libraryResourceStrings":8,"events":undefined,"fs":undefined,"path":undefined,"q":18}],12:[function(require,module,exports){
"use strict";
var common_1 = require('./common');
var libraryResourceStrings_1 = require('./libraryResourceStrings');
// set the default host ID to be some guid to prevent name collisions with user specified host IDs
var defaultHostId = 'C94B8CFE-E3FD-4BAF-A941-2866DBB566FE';
var ServiceModuleInfo = (function () {
    function ServiceModuleInfo(source) {
        if (!source) {
            throw new common_1.ServiceHubError(common_1.ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variableIsNotDefined('source'));
        }
        this.name = source.name;
        this.host = source.host;
        this.hostId = source.hostId;
        if (!this.hostId) {
            this.hostId = defaultHostId;
        }
        this.hostGroupAllowed = source.hostGroupAllowed;
        this.serviceBaseDirectory = source.serviceBaseDirectory;
        this.entryPoint = source.entryPoint;
        if (!source.host) {
            throw new common_1.ServiceHubError(common_1.ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variableIsNotDefined('host'));
        }
        if (!this.entryPoint) {
            throw new common_1.ServiceHubError(common_1.ErrorKind.InvalidArgument, libraryResourceStrings_1.LibraryResourceStrings.variableIsNotDefined('entryPoint'));
        }
    }
    return ServiceModuleInfo;
}());
exports.ServiceModuleInfo = ServiceModuleInfo;


},{"./common":1,"./libraryResourceStrings":8}],13:[function(require,module,exports){
"use strict";var errors={UNKNOWN:{errno:-1,code:"UNKNOWN",description:"unknown error"},OK:{errno:0,code:"OK",description:"success"},EOF:{errno:1,code:"EOF",description:"end of file"},EADDRINFO:{errno:2,code:"EADDRINFO",description:"getaddrinfo error"},EACCES:{errno:3,code:"EACCES",description:"permission denied"},EAGAIN:{errno:4,code:"EAGAIN",description:"no more processes"},EADDRINUSE:{errno:5,code:"EADDRINUSE",description:"address already in use"},EADDRNOTAVAIL:{errno:6,code:"EADDRNOTAVAIL",description:""},EAFNOSUPPORT:{errno:7,code:"EAFNOSUPPORT",description:""},EALREADY:{errno:8,code:"EALREADY",description:""},EBADF:{errno:9,code:"EBADF",description:"bad file descriptor"},EBUSY:{errno:10,code:"EBUSY",description:"resource busy or locked"},ECONNABORTED:{errno:11,code:"ECONNABORTED",description:"software caused connection abort"},ECONNREFUSED:{errno:12,code:"ECONNREFUSED",description:"connection refused"},ECONNRESET:{errno:13,code:"ECONNRESET",description:"connection reset by peer"},EDESTADDRREQ:{errno:14,code:"EDESTADDRREQ",description:"destination address required"},EFAULT:{errno:15,code:"EFAULT",description:"bad address in system call argument"},EHOSTUNREACH:{errno:16,code:"EHOSTUNREACH",description:"host is unreachable"},EINTR:{errno:17,code:"EINTR",description:"interrupted system call"},EINVAL:{errno:18,code:"EINVAL",description:"invalid argument"},EISCONN:{errno:19,code:"EISCONN",description:"socket is already connected"},EMFILE:{errno:20,code:"EMFILE",description:"too many open files"},EMSGSIZE:{errno:21,code:"EMSGSIZE",description:"message too long"},ENETDOWN:{errno:22,code:"ENETDOWN",description:"network is down"},ENETUNREACH:{errno:23,code:"ENETUNREACH",description:"network is unreachable"},ENFILE:{errno:24,code:"ENFILE",description:"file table overflow"},ENOBUFS:{errno:25,code:"ENOBUFS",description:"no buffer space available"},ENOMEM:{errno:26,code:"ENOMEM",description:"not enough memory"},ENOTDIR:{errno:27,code:"ENOTDIR",description:"not a directory"},EISDIR:{errno:28,code:"EISDIR",description:"illegal operation on a directory"},ENONET:{errno:29,code:"ENONET",description:"machine is not on the network"},ENOTCONN:{errno:31,code:"ENOTCONN",description:"socket is not connected"},ENOTSOCK:{errno:32,code:"ENOTSOCK",description:"socket operation on non-socket"},ENOTSUP:{errno:33,code:"ENOTSUP",description:"operation not supported on socket"},ENOENT:{errno:34,code:"ENOENT",description:"no such file or directory"},ENOSYS:{errno:35,code:"ENOSYS",description:"function not implemented"},EPIPE:{errno:36,code:"EPIPE",description:"broken pipe"},EPROTO:{errno:37,code:"EPROTO",description:"protocol error"},EPROTONOSUPPORT:{errno:38,code:"EPROTONOSUPPORT",description:"protocol not supported"},EPROTOTYPE:{errno:39,code:"EPROTOTYPE",description:"protocol wrong type for socket"},ETIMEDOUT:{errno:40,code:"ETIMEDOUT",description:"connection timed out"},ECHARSET:{errno:41,code:"ECHARSET",description:""},EAIFAMNOSUPPORT:{errno:42,code:"EAIFAMNOSUPPORT",description:""},EAISERVICE:{errno:44,code:"EAISERVICE",description:""},EAISOCKTYPE:{errno:45,code:"EAISOCKTYPE",description:""},ESHUTDOWN:{errno:46,code:"ESHUTDOWN",description:""},EEXIST:{errno:47,code:"EEXIST",description:"file already exists"},ESRCH:{errno:48,code:"ESRCH",description:"no such process"},ENAMETOOLONG:{errno:49,code:"ENAMETOOLONG",description:"name too long"},EPERM:{errno:50,code:"EPERM",description:"operation not permitted"},ELOOP:{errno:51,code:"ELOOP",description:"too many symbolic links encountered"},EXDEV:{errno:52,code:"EXDEV",description:"cross-device link not permitted"},ENOTEMPTY:{errno:53,code:"ENOTEMPTY",description:"directory not empty"},ENOSPC:{errno:54,code:"ENOSPC",description:"no space left on device"},EIO:{errno:55,code:"EIO",description:"i/o error"},EROFS:{errno:56,code:"EROFS",description:"read-only file system"},ENODEV:{errno:57,code:"ENODEV",description:"no such device"},ESPIPE:{errno:58,code:"ESPIPE",description:"invalid seek"},ECANCELED:{errno:59,code:"ECANCELED",description:"operation canceled"}},codes={"-1":"UNKNOWN",0:"OK",1:"EOF",2:"EADDRINFO",3:"EACCES",4:"EAGAIN",5:"EADDRINUSE",6:"EADDRNOTAVAIL",7:"EAFNOSUPPORT",8:"EALREADY",9:"EBADF",10:"EBUSY",11:"ECONNABORTED",12:"ECONNREFUSED",13:"ECONNRESET",14:"EDESTADDRREQ",15:"EFAULT",16:"EHOSTUNREACH",17:"EINTR",18:"EINVAL",19:"EISCONN",20:"EMFILE",21:"EMSGSIZE",22:"ENETDOWN",23:"ENETUNREACH",24:"ENFILE",25:"ENOBUFS",26:"ENOMEM",27:"ENOTDIR",28:"EISDIR",29:"ENONET",31:"ENOTCONN",32:"ENOTSOCK",33:"ENOTSUP",34:"ENOENT",35:"ENOSYS",36:"EPIPE",37:"EPROTO",38:"EPROTONOSUPPORT",39:"EPROTOTYPE",40:"ETIMEDOUT",41:"ECHARSET",42:"EAIFAMNOSUPPORT",44:"EAISERVICE",45:"EAISOCKTYPE",46:"ESHUTDOWN",47:"EEXIST",48:"ESRCH",49:"ENAMETOOLONG",50:"EPERM",51:"ELOOP",52:"EXDEV",53:"ENOTEMPTY",54:"ENOSPC",55:"EIO",56:"EROFS",57:"ENODEV",58:"ESPIPE",59:"ECANCELED"},nextAvailableErrno=100;errors.create=function(a,b,c){var d={errno:a,code:b,description:c};errors[b]=d,codes[a]=b},errors.get=function(a,b){var c,d=typeof a;d==="number"?c=errors[codes[a]]:d==="string"?c=errors[a]:c=a;var e=new Error(c.code);e.errno=c.errno,e.code=c.code;var f=c.description;if(b)for(var g in b)f=f.replace(new RegExp("\\{"+g+"\\}","g"),b[g]);return e.description=f,e},errors.getNextAvailableErrno=function(){var a=nextAvailableErrno;return nextAvailableErrno++,a},module.exports=errors;
},{}],14:[function(require,module,exports){
/*
 * js-sha256 v0.3.0
 * https://github.com/emn178/js-sha256
 *
 * Copyright 2014-2015, emn178@gmail.com
 *
 * Licensed under the MIT license:
 * http://www.opensource.org/licenses/MIT
 */
;(function(root, undefined) {
  'use strict';

  var NODE_JS = typeof(module) != 'undefined';
  if(NODE_JS) {
    root = global;
  }
  var TYPED_ARRAY = typeof(Uint8Array) != 'undefined';
  var HEX_CHARS = '0123456789abcdef'.split('');
  var EXTRA = [-2147483648, 8388608, 32768, 128];
  var SHIFT = [24, 16, 8, 0];
  var K =[0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
          0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
          0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
          0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
          0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
          0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
          0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
          0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2];

  var blocks = [];

  var sha224 = function(message) {
    return sha256(message, true);
  };

  var sha256 = function(message, is224) {
    var notString = typeof(message) != 'string';
    if(notString && message.constructor == root.ArrayBuffer) {
      message = new Uint8Array(message);
    }

    var h0, h1, h2, h3, h4, h5, h6, h7, block, code, first = true, end = false,
        i, j, index = 0, start = 0, bytes = 0, length = message.length,
        s0, s1, maj, t1, t2, ch, ab, da, cd, bc;

    if(is224) {
      h0 = 0xc1059ed8;
      h1 = 0x367cd507;
      h2 = 0x3070dd17;
      h3 = 0xf70e5939;
      h4 = 0xffc00b31;
      h5 = 0x68581511;
      h6 = 0x64f98fa7;
      h7 = 0xbefa4fa4;
    } else { // 256
      h0 = 0x6a09e667;
      h1 = 0xbb67ae85;
      h2 = 0x3c6ef372;
      h3 = 0xa54ff53a;
      h4 = 0x510e527f;
      h5 = 0x9b05688c;
      h6 = 0x1f83d9ab;
      h7 = 0x5be0cd19;
    }
    block = 0;
    do {
      blocks[0] = block;
      blocks[16] = blocks[1] = blocks[2] = blocks[3] =
      blocks[4] = blocks[5] = blocks[6] = blocks[7] =
      blocks[8] = blocks[9] = blocks[10] = blocks[11] =
      blocks[12] = blocks[13] = blocks[14] = blocks[15] = 0;
      if(notString) {
        for (i = start;index < length && i < 64; ++index) {
          blocks[i >> 2] |= message[index] << SHIFT[i++ & 3];
        }
      } else {
        for (i = start;index < length && i < 64; ++index) {
          code = message.charCodeAt(index);
          if (code < 0x80) {
            blocks[i >> 2] |= code << SHIFT[i++ & 3];
          } else if (code < 0x800) {
            blocks[i >> 2] |= (0xc0 | (code >> 6)) << SHIFT[i++ & 3];
            blocks[i >> 2] |= (0x80 | (code & 0x3f)) << SHIFT[i++ & 3];
          } else if (code < 0xd800 || code >= 0xe000) {
            blocks[i >> 2] |= (0xe0 | (code >> 12)) << SHIFT[i++ & 3];
            blocks[i >> 2] |= (0x80 | ((code >> 6) & 0x3f)) << SHIFT[i++ & 3];
            blocks[i >> 2] |= (0x80 | (code & 0x3f)) << SHIFT[i++ & 3];
          } else {
            code = 0x10000 + (((code & 0x3ff) << 10) | (message.charCodeAt(++index) & 0x3ff));
            blocks[i >> 2] |= (0xf0 | (code >> 18)) << SHIFT[i++ & 3];
            blocks[i >> 2] |= (0x80 | ((code >> 12) & 0x3f)) << SHIFT[i++ & 3];
            blocks[i >> 2] |= (0x80 | ((code >> 6) & 0x3f)) << SHIFT[i++ & 3];
            blocks[i >> 2] |= (0x80 | (code & 0x3f)) << SHIFT[i++ & 3];
          }
        }
      }
      bytes += i - start;
      start = i - 64;
      if(index == length) {
        blocks[i >> 2] |= EXTRA[i & 3];
        ++index;
      }
      block = blocks[16];
      if(index > length && i < 56) {
        blocks[15] = bytes << 3;
        end = true;
      }

      var a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;
      for(j = 16;j < 64;++j) {
        // rightrotate
        t1 = blocks[j - 15];
        s0 = ((t1 >>> 7) | (t1 << 25)) ^ ((t1 >>> 18) | (t1 << 14)) ^ (t1 >>> 3);
        t1 = blocks[j - 2];
        s1 = ((t1 >>> 17) | (t1 << 15)) ^ ((t1 >>> 19) | (t1 << 13)) ^ (t1 >>> 10);
        blocks[j] = blocks[j - 16] + s0 + blocks[j - 7] + s1 << 0;
      }

      bc = b & c;
      for(j = 0;j < 64;j += 4) {
        if(first) {
          if(is224) {
            ab = 300032;
            t1 = blocks[0] - 1413257819;
            h = t1 - 150054599 << 0;
            d = t1 + 24177077 << 0;
          } else {
            ab = 704751109;
            t1 = blocks[0] - 210244248;
            h = t1 - 1521486534 << 0;
            d = t1 + 143694565 << 0;
          }
          first = false;
        } else {
          s0 = ((a >>> 2) | (a << 30)) ^ ((a >>> 13) | (a << 19)) ^ ((a >>> 22) | (a << 10));
          s1 = ((e >>> 6) | (e << 26)) ^ ((e >>> 11) | (e << 21)) ^ ((e >>> 25) | (e << 7));
          ab = a & b;
          maj = ab ^ (a & c) ^ bc;
          ch = (e & f) ^ (~e & g);
          t1 = h + s1 + ch + K[j] + blocks[j];
          t2 = s0 + maj;
          h = d + t1 << 0;
          d = t1 + t2 << 0;
        }
        s0 = ((d >>> 2) | (d << 30)) ^ ((d >>> 13) | (d << 19)) ^ ((d >>> 22) | (d << 10));
        s1 = ((h >>> 6) | (h << 26)) ^ ((h >>> 11) | (h << 21)) ^ ((h >>> 25) | (h << 7));
        da = d & a;
        maj = da ^ (d & b) ^ ab;
        ch = (h & e) ^ (~h & f);
        t1 = g + s1 + ch + K[j + 1] + blocks[j + 1];
        t2 = s0 + maj;
        g = c + t1 << 0;
        c = t1 + t2 << 0;
        s0 = ((c >>> 2) | (c << 30)) ^ ((c >>> 13) | (c << 19)) ^ ((c >>> 22) | (c << 10));
        s1 = ((g >>> 6) | (g << 26)) ^ ((g >>> 11) | (g << 21)) ^ ((g >>> 25) | (g << 7));
        cd = c & d;
        maj = cd ^ (c & a) ^ da;
        ch = (g & h) ^ (~g & e);
        t1 = f + s1 + ch + K[j + 2] + blocks[j + 2];
        t2 = s0 + maj;
        f = b + t1 << 0;
        b = t1 + t2 << 0;
        s0 = ((b >>> 2) | (b << 30)) ^ ((b >>> 13) | (b << 19)) ^ ((b >>> 22) | (b << 10));
        s1 = ((f >>> 6) | (f << 26)) ^ ((f >>> 11) | (f << 21)) ^ ((f >>> 25) | (f << 7));
        bc = b & c;
        maj = bc ^ (b & d) ^ cd;
        ch = (f & g) ^ (~f & h);
        t1 = e + s1 + ch + K[j + 3] + blocks[j + 3];
        t2 = s0 + maj;
        e = a + t1 << 0;
        a = t1 + t2 << 0;
      }

      h0 = h0 + a << 0;
      h1 = h1 + b << 0;
      h2 = h2 + c << 0;
      h3 = h3 + d << 0;
      h4 = h4 + e << 0;
      h5 = h5 + f << 0;
      h6 = h6 + g << 0;
      h7 = h7 + h << 0;
    } while(!end);

    var hex = HEX_CHARS[(h0 >> 28) & 0x0F] + HEX_CHARS[(h0 >> 24) & 0x0F] +
              HEX_CHARS[(h0 >> 20) & 0x0F] + HEX_CHARS[(h0 >> 16) & 0x0F] +
              HEX_CHARS[(h0 >> 12) & 0x0F] + HEX_CHARS[(h0 >> 8) & 0x0F] +
              HEX_CHARS[(h0 >> 4) & 0x0F] + HEX_CHARS[h0 & 0x0F] +
              HEX_CHARS[(h1 >> 28) & 0x0F] + HEX_CHARS[(h1 >> 24) & 0x0F] +
              HEX_CHARS[(h1 >> 20) & 0x0F] + HEX_CHARS[(h1 >> 16) & 0x0F] +
              HEX_CHARS[(h1 >> 12) & 0x0F] + HEX_CHARS[(h1 >> 8) & 0x0F] +
              HEX_CHARS[(h1 >> 4) & 0x0F] + HEX_CHARS[h1 & 0x0F] +
              HEX_CHARS[(h2 >> 28) & 0x0F] + HEX_CHARS[(h2 >> 24) & 0x0F] +
              HEX_CHARS[(h2 >> 20) & 0x0F] + HEX_CHARS[(h2 >> 16) & 0x0F] +
              HEX_CHARS[(h2 >> 12) & 0x0F] + HEX_CHARS[(h2 >> 8) & 0x0F] +
              HEX_CHARS[(h2 >> 4) & 0x0F] + HEX_CHARS[h2 & 0x0F] +
              HEX_CHARS[(h3 >> 28) & 0x0F] + HEX_CHARS[(h3 >> 24) & 0x0F] +
              HEX_CHARS[(h3 >> 20) & 0x0F] + HEX_CHARS[(h3 >> 16) & 0x0F] +
              HEX_CHARS[(h3 >> 12) & 0x0F] + HEX_CHARS[(h3 >> 8) & 0x0F] +
              HEX_CHARS[(h3 >> 4) & 0x0F] + HEX_CHARS[h3 & 0x0F] +
              HEX_CHARS[(h4 >> 28) & 0x0F] + HEX_CHARS[(h4 >> 24) & 0x0F] +
              HEX_CHARS[(h4 >> 20) & 0x0F] + HEX_CHARS[(h4 >> 16) & 0x0F] +
              HEX_CHARS[(h4 >> 12) & 0x0F] + HEX_CHARS[(h4 >> 8) & 0x0F] +
              HEX_CHARS[(h4 >> 4) & 0x0F] + HEX_CHARS[h4 & 0x0F] +
              HEX_CHARS[(h5 >> 28) & 0x0F] + HEX_CHARS[(h5 >> 24) & 0x0F] +
              HEX_CHARS[(h5 >> 20) & 0x0F] + HEX_CHARS[(h5 >> 16) & 0x0F] +
              HEX_CHARS[(h5 >> 12) & 0x0F] + HEX_CHARS[(h5 >> 8) & 0x0F] +
              HEX_CHARS[(h5 >> 4) & 0x0F] + HEX_CHARS[h5 & 0x0F] +
              HEX_CHARS[(h6 >> 28) & 0x0F] + HEX_CHARS[(h6 >> 24) & 0x0F] +
              HEX_CHARS[(h6 >> 20) & 0x0F] + HEX_CHARS[(h6 >> 16) & 0x0F] +
              HEX_CHARS[(h6 >> 12) & 0x0F] + HEX_CHARS[(h6 >> 8) & 0x0F] +
              HEX_CHARS[(h6 >> 4) & 0x0F] + HEX_CHARS[h6 & 0x0F];
    if(!is224) {
      hex += HEX_CHARS[(h7 >> 28) & 0x0F] + HEX_CHARS[(h7 >> 24) & 0x0F] +
             HEX_CHARS[(h7 >> 20) & 0x0F] + HEX_CHARS[(h7 >> 16) & 0x0F] +
             HEX_CHARS[(h7 >> 12) & 0x0F] + HEX_CHARS[(h7 >> 8) & 0x0F] +
             HEX_CHARS[(h7 >> 4) & 0x0F] + HEX_CHARS[h7 & 0x0F];
    }
    return hex;
  };
  
  if(!root.JS_SHA256_TEST && NODE_JS) {
    sha256.sha256 = sha256;
    sha256.sha224 = sha224;
    module.exports = sha256;
  } else if(root) {
    root.sha256 = sha256;
    root.sha224 = sha224;
  }
}(this));

},{}],15:[function(require,module,exports){
// mkdir-parents.js

'use strict';

var fs = require('fs');
var path = require('path');


//######################################################################
/**
 * Function: make directory with parents recursively (async)
 * Param   : dir: path to make directory
 *           [mode]: {optional} file mode to make directory
 *           [cb]: {optional} callback(err) function
 */
function mkdirParents(dir, mode, cb) {
  // check arguments
  if (typeof dir !== 'string')
    throw new Error('mkdirParents: directory path required');

  if (typeof mode === 'function') {
    cb = mode;
    mode = undefined;
  }

  if (mode !== undefined && typeof mode !== 'number')
    throw new Error('mkdirParents: mode must be a number');

  if (cb !== undefined && typeof cb !== 'function')
    throw new Error('mkdirParents: callback must be function');

  dir = path.resolve(dir);

  var ctx = this, called, results;

  // local variables
  var dirList = []; // directories that we have to make directory

  fs.exists(dir, existsCallback);

  // fs.exists callback...
  function existsCallback(exists) {
    if (exists) {
      return mkdirCallback(null);
    }

    // if dir does not exist, then we have to make directory
    dirList.push(dir);
    dir = path.resolve(dir, '..');

    return fs.exists(dir, existsCallback);
  } // existsCallback

  // fs.mkdir callback...
  function mkdirCallback(err) {
    if (err && err.code !== 'EEXIST') {
      return mkdirParentsCallback(err);
    }

    dir = dirList.pop();
    if (!dir) {
      return mkdirParentsCallback(null);
    }

    return fs.mkdir(dir, mode, mkdirCallback);
  } // mkdirCallback

  // mkdirParentsCallback(err)
  function mkdirParentsCallback(err) {
    if (err && err.code === 'EEXIST') err = arguments[0] = null;
    if (!results) results = arguments;
    if (!cb || called) return;
    called = true;
    cb.apply(ctx, results);
  } // mkdirParentsCallback

  // return mkdirParentsYieldable
  return function mkdirParentsYieldable(fn) {
    if (!cb) cb = fn;
    if (!results || called) return;
    called = true;
    cb.apply(ctx, results);
  }; // mkdirParentsYieldable

} // mkdirParents


//######################################################################
/**
 * Function: make directory with parents recursively (sync)
 * Param   : dir: path to make directory
 *           mode: file mode to make directory
 */
function mkdirParentsSync(dir, mode) {
  // check arguments
  if (typeof dir !== 'string')
    throw new Error('mkdirParentsSync: directory path required');

  if (mode !== undefined && typeof mode !== 'number')
    throw new Error('mkdirParents: mode must be a number');

  dir = path.resolve(dir);

  var dirList = [];
  while (!fs.existsSync(dir)) {
    dirList.push(dir);
    dir = path.resolve(dir, '..');
  }

  while (dir = dirList.pop()) {
    try {
      fs.mkdirSync(dir, mode);
    } catch (err) {
      if (err && err.code !== 'EEXIST') throw err;
    }
  }

} // mkdirParentsSync


exports = module.exports = mkdirParents;
exports.mkdirParents     = mkdirParents;
exports.mkdirParentsSync = mkdirParentsSync;
exports.sync             = mkdirParentsSync;

},{"fs":undefined,"path":undefined}],16:[function(require,module,exports){
'use strict';
var os = require('os');

function homedir() {
	var env = process.env;
	var home = env.HOME;
	var user = env.LOGNAME || env.USER || env.LNAME || env.USERNAME;

	if (process.platform === 'win32') {
		return env.USERPROFILE || env.HOMEDRIVE + env.HOMEPATH || home || null;
	}

	if (process.platform === 'darwin') {
		return home || (user ? '/Users/' + user : null);
	}

	if (process.platform === 'linux') {
		return home || (process.getuid() === 0 ? '/root' : (user ? '/home/' + user : null));
	}

	return home || null;
}

module.exports = typeof os.homedir === 'function' ? os.homedir : homedir;

},{"os":undefined}],17:[function(require,module,exports){
'use strict';
var isWindows = process.platform === 'win32';
var trailingSlashRe = isWindows ? /[^:]\\$/ : /.\/$/;

// https://github.com/nodejs/io.js/blob/3e7a14381497a3b73dda68d05b5130563cdab420/lib/os.js#L25-L43
module.exports = function () {
	var path;

	if (isWindows) {
		path = process.env.TEMP ||
			process.env.TMP ||
			(process.env.SystemRoot || process.env.windir) + '\\temp';
	} else {
		path = process.env.TMPDIR ||
			process.env.TMP ||
			process.env.TEMP ||
			'/tmp';
	}

	if (trailingSlashRe.test(path)) {
		path = path.slice(0, -1);
	}

	return path;
};

},{}],18:[function(require,module,exports){
(function (setImmediate){
// vim:ts=4:sts=4:sw=4:
/*!
 *
 * Copyright 2009-2012 Kris Kowal under the terms of the MIT
 * license found at http://github.com/kriskowal/q/raw/master/LICENSE
 *
 * With parts by Tyler Close
 * Copyright 2007-2009 Tyler Close under the terms of the MIT X license found
 * at http://www.opensource.org/licenses/mit-license.html
 * Forked at ref_send.js version: 2009-05-11
 *
 * With parts by Mark Miller
 * Copyright (C) 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

(function (definition) {
    "use strict";

    // This file will function properly as a <script> tag, or a module
    // using CommonJS and NodeJS or RequireJS module formats.  In
    // Common/Node/RequireJS, the module exports the Q API and when
    // executed as a simple <script>, it creates a Q global instead.

    // Montage Require
    if (typeof bootstrap === "function") {
        bootstrap("promise", definition);

    // CommonJS
    } else if (typeof exports === "object" && typeof module === "object") {
        module.exports = definition();

    // RequireJS
    } else if (typeof define === "function" && define.amd) {
        define(definition);

    // SES (Secure EcmaScript)
    } else if (typeof ses !== "undefined") {
        if (!ses.ok()) {
            return;
        } else {
            ses.makeQ = definition;
        }

    // <script>
    } else if (typeof window !== "undefined" || typeof self !== "undefined") {
        // Prefer window over self for add-on scripts. Use self for
        // non-windowed contexts.
        var global = typeof window !== "undefined" ? window : self;

        // Get the `window` object, save the previous Q global
        // and initialize Q as a global.
        var previousQ = global.Q;
        global.Q = definition();

        // Add a noConflict function so Q can be removed from the
        // global namespace.
        global.Q.noConflict = function () {
            global.Q = previousQ;
            return this;
        };

    } else {
        throw new Error("This environment was not anticipated by Q. Please file a bug.");
    }

})(function () {
"use strict";

var hasStacks = false;
try {
    throw new Error();
} catch (e) {
    hasStacks = !!e.stack;
}

// All code after this point will be filtered from stack traces reported
// by Q.
var qStartingLine = captureLine();
var qFileName;

// shims

// used for fallback in "allResolved"
var noop = function () {};

// Use the fastest possible means to execute a task in a future turn
// of the event loop.
var nextTick =(function () {
    // linked list of tasks (single, with head node)
    var head = {task: void 0, next: null};
    var tail = head;
    var flushing = false;
    var requestTick = void 0;
    var isNodeJS = false;
    // queue for late tasks, used by unhandled rejection tracking
    var laterQueue = [];

    function flush() {
        /* jshint loopfunc: true */
        var task, domain;

        while (head.next) {
            head = head.next;
            task = head.task;
            head.task = void 0;
            domain = head.domain;

            if (domain) {
                head.domain = void 0;
                domain.enter();
            }
            runSingle(task, domain);

        }
        while (laterQueue.length) {
            task = laterQueue.pop();
            runSingle(task);
        }
        flushing = false;
    }
    // runs a single function in the async queue
    function runSingle(task, domain) {
        try {
            task();

        } catch (e) {
            if (isNodeJS) {
                // In node, uncaught exceptions are considered fatal errors.
                // Re-throw them synchronously to interrupt flushing!

                // Ensure continuation if the uncaught exception is suppressed
                // listening "uncaughtException" events (as domains does).
                // Continue in next event to avoid tick recursion.
                if (domain) {
                    domain.exit();
                }
                setTimeout(flush, 0);
                if (domain) {
                    domain.enter();
                }

                throw e;

            } else {
                // In browsers, uncaught exceptions are not fatal.
                // Re-throw them asynchronously to avoid slow-downs.
                setTimeout(function () {
                    throw e;
                }, 0);
            }
        }

        if (domain) {
            domain.exit();
        }
    }

    nextTick = function (task) {
        tail = tail.next = {
            task: task,
            domain: isNodeJS && process.domain,
            next: null
        };

        if (!flushing) {
            flushing = true;
            requestTick();
        }
    };

    if (typeof process === "object" &&
        process.toString() === "[object process]" && process.nextTick) {
        // Ensure Q is in a real Node environment, with a `process.nextTick`.
        // To see through fake Node environments:
        // * Mocha test runner - exposes a `process` global without a `nextTick`
        // * Browserify - exposes a `process.nexTick` function that uses
        //   `setTimeout`. In this case `setImmediate` is preferred because
        //    it is faster. Browserify's `process.toString()` yields
        //   "[object Object]", while in a real Node environment
        //   `process.nextTick()` yields "[object process]".
        isNodeJS = true;

        requestTick = function () {
            process.nextTick(flush);
        };

    } else if (typeof setImmediate === "function") {
        // In IE10, Node.js 0.9+, or https://github.com/NobleJS/setImmediate
        if (typeof window !== "undefined") {
            requestTick = setImmediate.bind(window, flush);
        } else {
            requestTick = function () {
                setImmediate(flush);
            };
        }

    } else if (typeof MessageChannel !== "undefined") {
        // modern browsers
        // http://www.nonblocking.io/2011/06/windownexttick.html
        var channel = new MessageChannel();
        // At least Safari Version 6.0.5 (8536.30.1) intermittently cannot create
        // working message ports the first time a page loads.
        channel.port1.onmessage = function () {
            requestTick = requestPortTick;
            channel.port1.onmessage = flush;
            flush();
        };
        var requestPortTick = function () {
            // Opera requires us to provide a message payload, regardless of
            // whether we use it.
            channel.port2.postMessage(0);
        };
        requestTick = function () {
            setTimeout(flush, 0);
            requestPortTick();
        };

    } else {
        // old browsers
        requestTick = function () {
            setTimeout(flush, 0);
        };
    }
    // runs a task after all other tasks have been run
    // this is useful for unhandled rejection tracking that needs to happen
    // after all `then`d tasks have been run.
    nextTick.runAfter = function (task) {
        laterQueue.push(task);
        if (!flushing) {
            flushing = true;
            requestTick();
        }
    };
    return nextTick;
})();

// Attempt to make generics safe in the face of downstream
// modifications.
// There is no situation where this is necessary.
// If you need a security guarantee, these primordials need to be
// deeply frozen anyway, and if you don’t need a security guarantee,
// this is just plain paranoid.
// However, this **might** have the nice side-effect of reducing the size of
// the minified code by reducing x.call() to merely x()
// See Mark Miller’s explanation of what this does.
// http://wiki.ecmascript.org/doku.php?id=conventions:safe_meta_programming
var call = Function.call;
function uncurryThis(f) {
    return function () {
        return call.apply(f, arguments);
    };
}
// This is equivalent, but slower:
// uncurryThis = Function_bind.bind(Function_bind.call);
// http://jsperf.com/uncurrythis

var array_slice = uncurryThis(Array.prototype.slice);

var array_reduce = uncurryThis(
    Array.prototype.reduce || function (callback, basis) {
        var index = 0,
            length = this.length;
        // concerning the initial value, if one is not provided
        if (arguments.length === 1) {
            // seek to the first value in the array, accounting
            // for the possibility that is is a sparse array
            do {
                if (index in this) {
                    basis = this[index++];
                    break;
                }
                if (++index >= length) {
                    throw new TypeError();
                }
            } while (1);
        }
        // reduce
        for (; index < length; index++) {
            // account for the possibility that the array is sparse
            if (index in this) {
                basis = callback(basis, this[index], index);
            }
        }
        return basis;
    }
);

var array_indexOf = uncurryThis(
    Array.prototype.indexOf || function (value) {
        // not a very good shim, but good enough for our one use of it
        for (var i = 0; i < this.length; i++) {
            if (this[i] === value) {
                return i;
            }
        }
        return -1;
    }
);

var array_map = uncurryThis(
    Array.prototype.map || function (callback, thisp) {
        var self = this;
        var collect = [];
        array_reduce(self, function (undefined, value, index) {
            collect.push(callback.call(thisp, value, index, self));
        }, void 0);
        return collect;
    }
);

var object_create = Object.create || function (prototype) {
    function Type() { }
    Type.prototype = prototype;
    return new Type();
};

var object_hasOwnProperty = uncurryThis(Object.prototype.hasOwnProperty);

var object_keys = Object.keys || function (object) {
    var keys = [];
    for (var key in object) {
        if (object_hasOwnProperty(object, key)) {
            keys.push(key);
        }
    }
    return keys;
};

var object_toString = uncurryThis(Object.prototype.toString);

function isObject(value) {
    return value === Object(value);
}

// generator related shims

// FIXME: Remove this function once ES6 generators are in SpiderMonkey.
function isStopIteration(exception) {
    return (
        object_toString(exception) === "[object StopIteration]" ||
        exception instanceof QReturnValue
    );
}

// FIXME: Remove this helper and Q.return once ES6 generators are in
// SpiderMonkey.
var QReturnValue;
if (typeof ReturnValue !== "undefined") {
    QReturnValue = ReturnValue;
} else {
    QReturnValue = function (value) {
        this.value = value;
    };
}

// long stack traces

var STACK_JUMP_SEPARATOR = "From previous event:";

function makeStackTraceLong(error, promise) {
    // If possible, transform the error stack trace by removing Node and Q
    // cruft, then concatenating with the stack trace of `promise`. See #57.
    if (hasStacks &&
        promise.stack &&
        typeof error === "object" &&
        error !== null &&
        error.stack &&
        error.stack.indexOf(STACK_JUMP_SEPARATOR) === -1
    ) {
        var stacks = [];
        for (var p = promise; !!p; p = p.source) {
            if (p.stack) {
                stacks.unshift(p.stack);
            }
        }
        stacks.unshift(error.stack);

        var concatedStacks = stacks.join("\n" + STACK_JUMP_SEPARATOR + "\n");
        error.stack = filterStackString(concatedStacks);
    }
}

function filterStackString(stackString) {
    var lines = stackString.split("\n");
    var desiredLines = [];
    for (var i = 0; i < lines.length; ++i) {
        var line = lines[i];

        if (!isInternalFrame(line) && !isNodeFrame(line) && line) {
            desiredLines.push(line);
        }
    }
    return desiredLines.join("\n");
}

function isNodeFrame(stackLine) {
    return stackLine.indexOf("(module.js:") !== -1 ||
           stackLine.indexOf("(node.js:") !== -1;
}

function getFileNameAndLineNumber(stackLine) {
    // Named functions: "at functionName (filename:lineNumber:columnNumber)"
    // In IE10 function name can have spaces ("Anonymous function") O_o
    var attempt1 = /at .+ \((.+):(\d+):(?:\d+)\)$/.exec(stackLine);
    if (attempt1) {
        return [attempt1[1], Number(attempt1[2])];
    }

    // Anonymous functions: "at filename:lineNumber:columnNumber"
    var attempt2 = /at ([^ ]+):(\d+):(?:\d+)$/.exec(stackLine);
    if (attempt2) {
        return [attempt2[1], Number(attempt2[2])];
    }

    // Firefox style: "function@filename:lineNumber or @filename:lineNumber"
    var attempt3 = /.*@(.+):(\d+)$/.exec(stackLine);
    if (attempt3) {
        return [attempt3[1], Number(attempt3[2])];
    }
}

function isInternalFrame(stackLine) {
    var fileNameAndLineNumber = getFileNameAndLineNumber(stackLine);

    if (!fileNameAndLineNumber) {
        return false;
    }

    var fileName = fileNameAndLineNumber[0];
    var lineNumber = fileNameAndLineNumber[1];

    return fileName === qFileName &&
        lineNumber >= qStartingLine &&
        lineNumber <= qEndingLine;
}

// discover own file name and line number range for filtering stack
// traces
function captureLine() {
    if (!hasStacks) {
        return;
    }

    try {
        throw new Error();
    } catch (e) {
        var lines = e.stack.split("\n");
        var firstLine = lines[0].indexOf("@") > 0 ? lines[1] : lines[2];
        var fileNameAndLineNumber = getFileNameAndLineNumber(firstLine);
        if (!fileNameAndLineNumber) {
            return;
        }

        qFileName = fileNameAndLineNumber[0];
        return fileNameAndLineNumber[1];
    }
}

function deprecate(callback, name, alternative) {
    return function () {
        if (typeof console !== "undefined" &&
            typeof console.warn === "function") {
            console.warn(name + " is deprecated, use " + alternative +
                         " instead.", new Error("").stack);
        }
        return callback.apply(callback, arguments);
    };
}

// end of shims
// beginning of real work

/**
 * Constructs a promise for an immediate reference, passes promises through, or
 * coerces promises from different systems.
 * @param value immediate reference or promise
 */
function Q(value) {
    // If the object is already a Promise, return it directly.  This enables
    // the resolve function to both be used to created references from objects,
    // but to tolerably coerce non-promises to promises.
    if (value instanceof Promise) {
        return value;
    }

    // assimilate thenables
    if (isPromiseAlike(value)) {
        return coerce(value);
    } else {
        return fulfill(value);
    }
}
Q.resolve = Q;

/**
 * Performs a task in a future turn of the event loop.
 * @param {Function} task
 */
Q.nextTick = nextTick;

/**
 * Controls whether or not long stack traces will be on
 */
Q.longStackSupport = false;

// enable long stacks if Q_DEBUG is set
if (typeof process === "object" && process && process.env && process.env.Q_DEBUG) {
    Q.longStackSupport = true;
}

/**
 * Constructs a {promise, resolve, reject} object.
 *
 * `resolve` is a callback to invoke with a more resolved value for the
 * promise. To fulfill the promise, invoke `resolve` with any value that is
 * not a thenable. To reject the promise, invoke `resolve` with a rejected
 * thenable, or invoke `reject` with the reason directly. To resolve the
 * promise to another thenable, thus putting it in the same state, invoke
 * `resolve` with that other thenable.
 */
Q.defer = defer;
function defer() {
    // if "messages" is an "Array", that indicates that the promise has not yet
    // been resolved.  If it is "undefined", it has been resolved.  Each
    // element of the messages array is itself an array of complete arguments to
    // forward to the resolved promise.  We coerce the resolution value to a
    // promise using the `resolve` function because it handles both fully
    // non-thenable values and other thenables gracefully.
    var messages = [], progressListeners = [], resolvedPromise;

    var deferred = object_create(defer.prototype);
    var promise = object_create(Promise.prototype);

    promise.promiseDispatch = function (resolve, op, operands) {
        var args = array_slice(arguments);
        if (messages) {
            messages.push(args);
            if (op === "when" && operands[1]) { // progress operand
                progressListeners.push(operands[1]);
            }
        } else {
            Q.nextTick(function () {
                resolvedPromise.promiseDispatch.apply(resolvedPromise, args);
            });
        }
    };

    // XXX deprecated
    promise.valueOf = function () {
        if (messages) {
            return promise;
        }
        var nearerValue = nearer(resolvedPromise);
        if (isPromise(nearerValue)) {
            resolvedPromise = nearerValue; // shorten chain
        }
        return nearerValue;
    };

    promise.inspect = function () {
        if (!resolvedPromise) {
            return { state: "pending" };
        }
        return resolvedPromise.inspect();
    };

    if (Q.longStackSupport && hasStacks) {
        try {
            throw new Error();
        } catch (e) {
            // NOTE: don't try to use `Error.captureStackTrace` or transfer the
            // accessor around; that causes memory leaks as per GH-111. Just
            // reify the stack trace as a string ASAP.
            //
            // At the same time, cut off the first line; it's always just
            // "[object Promise]\n", as per the `toString`.
            promise.stack = e.stack.substring(e.stack.indexOf("\n") + 1);
        }
    }

    // NOTE: we do the checks for `resolvedPromise` in each method, instead of
    // consolidating them into `become`, since otherwise we'd create new
    // promises with the lines `become(whatever(value))`. See e.g. GH-252.

    function become(newPromise) {
        resolvedPromise = newPromise;
        promise.source = newPromise;

        array_reduce(messages, function (undefined, message) {
            Q.nextTick(function () {
                newPromise.promiseDispatch.apply(newPromise, message);
            });
        }, void 0);

        messages = void 0;
        progressListeners = void 0;
    }

    deferred.promise = promise;
    deferred.resolve = function (value) {
        if (resolvedPromise) {
            return;
        }

        become(Q(value));
    };

    deferred.fulfill = function (value) {
        if (resolvedPromise) {
            return;
        }

        become(fulfill(value));
    };
    deferred.reject = function (reason) {
        if (resolvedPromise) {
            return;
        }

        become(reject(reason));
    };
    deferred.notify = function (progress) {
        if (resolvedPromise) {
            return;
        }

        array_reduce(progressListeners, function (undefined, progressListener) {
            Q.nextTick(function () {
                progressListener(progress);
            });
        }, void 0);
    };

    return deferred;
}

/**
 * Creates a Node-style callback that will resolve or reject the deferred
 * promise.
 * @returns a nodeback
 */
defer.prototype.makeNodeResolver = function () {
    var self = this;
    return function (error, value) {
        if (error) {
            self.reject(error);
        } else if (arguments.length > 2) {
            self.resolve(array_slice(arguments, 1));
        } else {
            self.resolve(value);
        }
    };
};

/**
 * @param resolver {Function} a function that returns nothing and accepts
 * the resolve, reject, and notify functions for a deferred.
 * @returns a promise that may be resolved with the given resolve and reject
 * functions, or rejected by a thrown exception in resolver
 */
Q.Promise = promise; // ES6
Q.promise = promise;
function promise(resolver) {
    if (typeof resolver !== "function") {
        throw new TypeError("resolver must be a function.");
    }
    var deferred = defer();
    try {
        resolver(deferred.resolve, deferred.reject, deferred.notify);
    } catch (reason) {
        deferred.reject(reason);
    }
    return deferred.promise;
}

promise.race = race; // ES6
promise.all = all; // ES6
promise.reject = reject; // ES6
promise.resolve = Q; // ES6

// XXX experimental.  This method is a way to denote that a local value is
// serializable and should be immediately dispatched to a remote upon request,
// instead of passing a reference.
Q.passByCopy = function (object) {
    //freeze(object);
    //passByCopies.set(object, true);
    return object;
};

Promise.prototype.passByCopy = function () {
    //freeze(object);
    //passByCopies.set(object, true);
    return this;
};

/**
 * If two promises eventually fulfill to the same value, promises that value,
 * but otherwise rejects.
 * @param x {Any*}
 * @param y {Any*}
 * @returns {Any*} a promise for x and y if they are the same, but a rejection
 * otherwise.
 *
 */
Q.join = function (x, y) {
    return Q(x).join(y);
};

Promise.prototype.join = function (that) {
    return Q([this, that]).spread(function (x, y) {
        if (x === y) {
            // TODO: "===" should be Object.is or equiv
            return x;
        } else {
            throw new Error("Can't join: not the same: " + x + " " + y);
        }
    });
};

/**
 * Returns a promise for the first of an array of promises to become settled.
 * @param answers {Array[Any*]} promises to race
 * @returns {Any*} the first promise to be settled
 */
Q.race = race;
function race(answerPs) {
    return promise(function (resolve, reject) {
        // Switch to this once we can assume at least ES5
        // answerPs.forEach(function (answerP) {
        //     Q(answerP).then(resolve, reject);
        // });
        // Use this in the meantime
        for (var i = 0, len = answerPs.length; i < len; i++) {
            Q(answerPs[i]).then(resolve, reject);
        }
    });
}

Promise.prototype.race = function () {
    return this.then(Q.race);
};

/**
 * Constructs a Promise with a promise descriptor object and optional fallback
 * function.  The descriptor contains methods like when(rejected), get(name),
 * set(name, value), post(name, args), and delete(name), which all
 * return either a value, a promise for a value, or a rejection.  The fallback
 * accepts the operation name, a resolver, and any further arguments that would
 * have been forwarded to the appropriate method above had a method been
 * provided with the proper name.  The API makes no guarantees about the nature
 * of the returned object, apart from that it is usable whereever promises are
 * bought and sold.
 */
Q.makePromise = Promise;
function Promise(descriptor, fallback, inspect) {
    if (fallback === void 0) {
        fallback = function (op) {
            return reject(new Error(
                "Promise does not support operation: " + op
            ));
        };
    }
    if (inspect === void 0) {
        inspect = function () {
            return {state: "unknown"};
        };
    }

    var promise = object_create(Promise.prototype);

    promise.promiseDispatch = function (resolve, op, args) {
        var result;
        try {
            if (descriptor[op]) {
                result = descriptor[op].apply(promise, args);
            } else {
                result = fallback.call(promise, op, args);
            }
        } catch (exception) {
            result = reject(exception);
        }
        if (resolve) {
            resolve(result);
        }
    };

    promise.inspect = inspect;

    // XXX deprecated `valueOf` and `exception` support
    if (inspect) {
        var inspected = inspect();
        if (inspected.state === "rejected") {
            promise.exception = inspected.reason;
        }

        promise.valueOf = function () {
            var inspected = inspect();
            if (inspected.state === "pending" ||
                inspected.state === "rejected") {
                return promise;
            }
            return inspected.value;
        };
    }

    return promise;
}

Promise.prototype.toString = function () {
    return "[object Promise]";
};

Promise.prototype.then = function (fulfilled, rejected, progressed) {
    var self = this;
    var deferred = defer();
    var done = false;   // ensure the untrusted promise makes at most a
                        // single call to one of the callbacks

    function _fulfilled(value) {
        try {
            return typeof fulfilled === "function" ? fulfilled(value) : value;
        } catch (exception) {
            return reject(exception);
        }
    }

    function _rejected(exception) {
        if (typeof rejected === "function") {
            makeStackTraceLong(exception, self);
            try {
                return rejected(exception);
            } catch (newException) {
                return reject(newException);
            }
        }
        return reject(exception);
    }

    function _progressed(value) {
        return typeof progressed === "function" ? progressed(value) : value;
    }

    Q.nextTick(function () {
        self.promiseDispatch(function (value) {
            if (done) {
                return;
            }
            done = true;

            deferred.resolve(_fulfilled(value));
        }, "when", [function (exception) {
            if (done) {
                return;
            }
            done = true;

            deferred.resolve(_rejected(exception));
        }]);
    });

    // Progress propagator need to be attached in the current tick.
    self.promiseDispatch(void 0, "when", [void 0, function (value) {
        var newValue;
        var threw = false;
        try {
            newValue = _progressed(value);
        } catch (e) {
            threw = true;
            if (Q.onerror) {
                Q.onerror(e);
            } else {
                throw e;
            }
        }

        if (!threw) {
            deferred.notify(newValue);
        }
    }]);

    return deferred.promise;
};

Q.tap = function (promise, callback) {
    return Q(promise).tap(callback);
};

/**
 * Works almost like "finally", but not called for rejections.
 * Original resolution value is passed through callback unaffected.
 * Callback may return a promise that will be awaited for.
 * @param {Function} callback
 * @returns {Q.Promise}
 * @example
 * doSomething()
 *   .then(...)
 *   .tap(console.log)
 *   .then(...);
 */
Promise.prototype.tap = function (callback) {
    callback = Q(callback);

    return this.then(function (value) {
        return callback.fcall(value).thenResolve(value);
    });
};

/**
 * Registers an observer on a promise.
 *
 * Guarantees:
 *
 * 1. that fulfilled and rejected will be called only once.
 * 2. that either the fulfilled callback or the rejected callback will be
 *    called, but not both.
 * 3. that fulfilled and rejected will not be called in this turn.
 *
 * @param value      promise or immediate reference to observe
 * @param fulfilled  function to be called with the fulfilled value
 * @param rejected   function to be called with the rejection exception
 * @param progressed function to be called on any progress notifications
 * @return promise for the return value from the invoked callback
 */
Q.when = when;
function when(value, fulfilled, rejected, progressed) {
    return Q(value).then(fulfilled, rejected, progressed);
}

Promise.prototype.thenResolve = function (value) {
    return this.then(function () { return value; });
};

Q.thenResolve = function (promise, value) {
    return Q(promise).thenResolve(value);
};

Promise.prototype.thenReject = function (reason) {
    return this.then(function () { throw reason; });
};

Q.thenReject = function (promise, reason) {
    return Q(promise).thenReject(reason);
};

/**
 * If an object is not a promise, it is as "near" as possible.
 * If a promise is rejected, it is as "near" as possible too.
 * If it’s a fulfilled promise, the fulfillment value is nearer.
 * If it’s a deferred promise and the deferred has been resolved, the
 * resolution is "nearer".
 * @param object
 * @returns most resolved (nearest) form of the object
 */

// XXX should we re-do this?
Q.nearer = nearer;
function nearer(value) {
    if (isPromise(value)) {
        var inspected = value.inspect();
        if (inspected.state === "fulfilled") {
            return inspected.value;
        }
    }
    return value;
}

/**
 * @returns whether the given object is a promise.
 * Otherwise it is a fulfilled value.
 */
Q.isPromise = isPromise;
function isPromise(object) {
    return object instanceof Promise;
}

Q.isPromiseAlike = isPromiseAlike;
function isPromiseAlike(object) {
    return isObject(object) && typeof object.then === "function";
}

/**
 * @returns whether the given object is a pending promise, meaning not
 * fulfilled or rejected.
 */
Q.isPending = isPending;
function isPending(object) {
    return isPromise(object) && object.inspect().state === "pending";
}

Promise.prototype.isPending = function () {
    return this.inspect().state === "pending";
};

/**
 * @returns whether the given object is a value or fulfilled
 * promise.
 */
Q.isFulfilled = isFulfilled;
function isFulfilled(object) {
    return !isPromise(object) || object.inspect().state === "fulfilled";
}

Promise.prototype.isFulfilled = function () {
    return this.inspect().state === "fulfilled";
};

/**
 * @returns whether the given object is a rejected promise.
 */
Q.isRejected = isRejected;
function isRejected(object) {
    return isPromise(object) && object.inspect().state === "rejected";
}

Promise.prototype.isRejected = function () {
    return this.inspect().state === "rejected";
};

//// BEGIN UNHANDLED REJECTION TRACKING

// This promise library consumes exceptions thrown in handlers so they can be
// handled by a subsequent promise.  The exceptions get added to this array when
// they are created, and removed when they are handled.  Note that in ES6 or
// shimmed environments, this would naturally be a `Set`.
var unhandledReasons = [];
var unhandledRejections = [];
var reportedUnhandledRejections = [];
var trackUnhandledRejections = true;

function resetUnhandledRejections() {
    unhandledReasons.length = 0;
    unhandledRejections.length = 0;

    if (!trackUnhandledRejections) {
        trackUnhandledRejections = true;
    }
}

function trackRejection(promise, reason) {
    if (!trackUnhandledRejections) {
        return;
    }
    if (typeof process === "object" && typeof process.emit === "function") {
        Q.nextTick.runAfter(function () {
            if (array_indexOf(unhandledRejections, promise) !== -1) {
                process.emit("unhandledRejection", reason, promise);
                reportedUnhandledRejections.push(promise);
            }
        });
    }

    unhandledRejections.push(promise);
    if (reason && typeof reason.stack !== "undefined") {
        unhandledReasons.push(reason.stack);
    } else {
        unhandledReasons.push("(no stack) " + reason);
    }
}

function untrackRejection(promise) {
    if (!trackUnhandledRejections) {
        return;
    }

    var at = array_indexOf(unhandledRejections, promise);
    if (at !== -1) {
        if (typeof process === "object" && typeof process.emit === "function") {
            Q.nextTick.runAfter(function () {
                var atReport = array_indexOf(reportedUnhandledRejections, promise);
                if (atReport !== -1) {
                    process.emit("rejectionHandled", unhandledReasons[at], promise);
                    reportedUnhandledRejections.splice(atReport, 1);
                }
            });
        }
        unhandledRejections.splice(at, 1);
        unhandledReasons.splice(at, 1);
    }
}

Q.resetUnhandledRejections = resetUnhandledRejections;

Q.getUnhandledReasons = function () {
    // Make a copy so that consumers can't interfere with our internal state.
    return unhandledReasons.slice();
};

Q.stopUnhandledRejectionTracking = function () {
    resetUnhandledRejections();
    trackUnhandledRejections = false;
};

resetUnhandledRejections();

//// END UNHANDLED REJECTION TRACKING

/**
 * Constructs a rejected promise.
 * @param reason value describing the failure
 */
Q.reject = reject;
function reject(reason) {
    var rejection = Promise({
        "when": function (rejected) {
            // note that the error has been handled
            if (rejected) {
                untrackRejection(this);
            }
            return rejected ? rejected(reason) : this;
        }
    }, function fallback() {
        return this;
    }, function inspect() {
        return { state: "rejected", reason: reason };
    });

    // Note that the reason has not been handled.
    trackRejection(rejection, reason);

    return rejection;
}

/**
 * Constructs a fulfilled promise for an immediate reference.
 * @param value immediate reference
 */
Q.fulfill = fulfill;
function fulfill(value) {
    return Promise({
        "when": function () {
            return value;
        },
        "get": function (name) {
            return value[name];
        },
        "set": function (name, rhs) {
            value[name] = rhs;
        },
        "delete": function (name) {
            delete value[name];
        },
        "post": function (name, args) {
            // Mark Miller proposes that post with no name should apply a
            // promised function.
            if (name === null || name === void 0) {
                return value.apply(void 0, args);
            } else {
                return value[name].apply(value, args);
            }
        },
        "apply": function (thisp, args) {
            return value.apply(thisp, args);
        },
        "keys": function () {
            return object_keys(value);
        }
    }, void 0, function inspect() {
        return { state: "fulfilled", value: value };
    });
}

/**
 * Converts thenables to Q promises.
 * @param promise thenable promise
 * @returns a Q promise
 */
function coerce(promise) {
    var deferred = defer();
    Q.nextTick(function () {
        try {
            promise.then(deferred.resolve, deferred.reject, deferred.notify);
        } catch (exception) {
            deferred.reject(exception);
        }
    });
    return deferred.promise;
}

/**
 * Annotates an object such that it will never be
 * transferred away from this process over any promise
 * communication channel.
 * @param object
 * @returns promise a wrapping of that object that
 * additionally responds to the "isDef" message
 * without a rejection.
 */
Q.master = master;
function master(object) {
    return Promise({
        "isDef": function () {}
    }, function fallback(op, args) {
        return dispatch(object, op, args);
    }, function () {
        return Q(object).inspect();
    });
}

/**
 * Spreads the values of a promised array of arguments into the
 * fulfillment callback.
 * @param fulfilled callback that receives variadic arguments from the
 * promised array
 * @param rejected callback that receives the exception if the promise
 * is rejected.
 * @returns a promise for the return value or thrown exception of
 * either callback.
 */
Q.spread = spread;
function spread(value, fulfilled, rejected) {
    return Q(value).spread(fulfilled, rejected);
}

Promise.prototype.spread = function (fulfilled, rejected) {
    return this.all().then(function (array) {
        return fulfilled.apply(void 0, array);
    }, rejected);
};

/**
 * The async function is a decorator for generator functions, turning
 * them into asynchronous generators.  Although generators are only part
 * of the newest ECMAScript 6 drafts, this code does not cause syntax
 * errors in older engines.  This code should continue to work and will
 * in fact improve over time as the language improves.
 *
 * ES6 generators are currently part of V8 version 3.19 with the
 * --harmony-generators runtime flag enabled.  SpiderMonkey has had them
 * for longer, but under an older Python-inspired form.  This function
 * works on both kinds of generators.
 *
 * Decorates a generator function such that:
 *  - it may yield promises
 *  - execution will continue when that promise is fulfilled
 *  - the value of the yield expression will be the fulfilled value
 *  - it returns a promise for the return value (when the generator
 *    stops iterating)
 *  - the decorated function returns a promise for the return value
 *    of the generator or the first rejected promise among those
 *    yielded.
 *  - if an error is thrown in the generator, it propagates through
 *    every following yield until it is caught, or until it escapes
 *    the generator function altogether, and is translated into a
 *    rejection for the promise returned by the decorated generator.
 */
Q.async = async;
function async(makeGenerator) {
    return function () {
        // when verb is "send", arg is a value
        // when verb is "throw", arg is an exception
        function continuer(verb, arg) {
            var result;

            // Until V8 3.19 / Chromium 29 is released, SpiderMonkey is the only
            // engine that has a deployed base of browsers that support generators.
            // However, SM's generators use the Python-inspired semantics of
            // outdated ES6 drafts.  We would like to support ES6, but we'd also
            // like to make it possible to use generators in deployed browsers, so
            // we also support Python-style generators.  At some point we can remove
            // this block.

            if (typeof StopIteration === "undefined") {
                // ES6 Generators
                try {
                    result = generator[verb](arg);
                } catch (exception) {
                    return reject(exception);
                }
                if (result.done) {
                    return Q(result.value);
                } else {
                    return when(result.value, callback, errback);
                }
            } else {
                // SpiderMonkey Generators
                // FIXME: Remove this case when SM does ES6 generators.
                try {
                    result = generator[verb](arg);
                } catch (exception) {
                    if (isStopIteration(exception)) {
                        return Q(exception.value);
                    } else {
                        return reject(exception);
                    }
                }
                return when(result, callback, errback);
            }
        }
        var generator = makeGenerator.apply(this, arguments);
        var callback = continuer.bind(continuer, "next");
        var errback = continuer.bind(continuer, "throw");
        return callback();
    };
}

/**
 * The spawn function is a small wrapper around async that immediately
 * calls the generator and also ends the promise chain, so that any
 * unhandled errors are thrown instead of forwarded to the error
 * handler. This is useful because it's extremely common to run
 * generators at the top-level to work with libraries.
 */
Q.spawn = spawn;
function spawn(makeGenerator) {
    Q.done(Q.async(makeGenerator)());
}

// FIXME: Remove this interface once ES6 generators are in SpiderMonkey.
/**
 * Throws a ReturnValue exception to stop an asynchronous generator.
 *
 * This interface is a stop-gap measure to support generator return
 * values in older Firefox/SpiderMonkey.  In browsers that support ES6
 * generators like Chromium 29, just use "return" in your generator
 * functions.
 *
 * @param value the return value for the surrounding generator
 * @throws ReturnValue exception with the value.
 * @example
 * // ES6 style
 * Q.async(function* () {
 *      var foo = yield getFooPromise();
 *      var bar = yield getBarPromise();
 *      return foo + bar;
 * })
 * // Older SpiderMonkey style
 * Q.async(function () {
 *      var foo = yield getFooPromise();
 *      var bar = yield getBarPromise();
 *      Q.return(foo + bar);
 * })
 */
Q["return"] = _return;
function _return(value) {
    throw new QReturnValue(value);
}

/**
 * The promised function decorator ensures that any promise arguments
 * are settled and passed as values (`this` is also settled and passed
 * as a value).  It will also ensure that the result of a function is
 * always a promise.
 *
 * @example
 * var add = Q.promised(function (a, b) {
 *     return a + b;
 * });
 * add(Q(a), Q(B));
 *
 * @param {function} callback The function to decorate
 * @returns {function} a function that has been decorated.
 */
Q.promised = promised;
function promised(callback) {
    return function () {
        return spread([this, all(arguments)], function (self, args) {
            return callback.apply(self, args);
        });
    };
}

/**
 * sends a message to a value in a future turn
 * @param object* the recipient
 * @param op the name of the message operation, e.g., "when",
 * @param args further arguments to be forwarded to the operation
 * @returns result {Promise} a promise for the result of the operation
 */
Q.dispatch = dispatch;
function dispatch(object, op, args) {
    return Q(object).dispatch(op, args);
}

Promise.prototype.dispatch = function (op, args) {
    var self = this;
    var deferred = defer();
    Q.nextTick(function () {
        self.promiseDispatch(deferred.resolve, op, args);
    });
    return deferred.promise;
};

/**
 * Gets the value of a property in a future turn.
 * @param object    promise or immediate reference for target object
 * @param name      name of property to get
 * @return promise for the property value
 */
Q.get = function (object, key) {
    return Q(object).dispatch("get", [key]);
};

Promise.prototype.get = function (key) {
    return this.dispatch("get", [key]);
};

/**
 * Sets the value of a property in a future turn.
 * @param object    promise or immediate reference for object object
 * @param name      name of property to set
 * @param value     new value of property
 * @return promise for the return value
 */
Q.set = function (object, key, value) {
    return Q(object).dispatch("set", [key, value]);
};

Promise.prototype.set = function (key, value) {
    return this.dispatch("set", [key, value]);
};

/**
 * Deletes a property in a future turn.
 * @param object    promise or immediate reference for target object
 * @param name      name of property to delete
 * @return promise for the return value
 */
Q.del = // XXX legacy
Q["delete"] = function (object, key) {
    return Q(object).dispatch("delete", [key]);
};

Promise.prototype.del = // XXX legacy
Promise.prototype["delete"] = function (key) {
    return this.dispatch("delete", [key]);
};

/**
 * Invokes a method in a future turn.
 * @param object    promise or immediate reference for target object
 * @param name      name of method to invoke
 * @param value     a value to post, typically an array of
 *                  invocation arguments for promises that
 *                  are ultimately backed with `resolve` values,
 *                  as opposed to those backed with URLs
 *                  wherein the posted value can be any
 *                  JSON serializable object.
 * @return promise for the return value
 */
// bound locally because it is used by other methods
Q.mapply = // XXX As proposed by "Redsandro"
Q.post = function (object, name, args) {
    return Q(object).dispatch("post", [name, args]);
};

Promise.prototype.mapply = // XXX As proposed by "Redsandro"
Promise.prototype.post = function (name, args) {
    return this.dispatch("post", [name, args]);
};

/**
 * Invokes a method in a future turn.
 * @param object    promise or immediate reference for target object
 * @param name      name of method to invoke
 * @param ...args   array of invocation arguments
 * @return promise for the return value
 */
Q.send = // XXX Mark Miller's proposed parlance
Q.mcall = // XXX As proposed by "Redsandro"
Q.invoke = function (object, name /*...args*/) {
    return Q(object).dispatch("post", [name, array_slice(arguments, 2)]);
};

Promise.prototype.send = // XXX Mark Miller's proposed parlance
Promise.prototype.mcall = // XXX As proposed by "Redsandro"
Promise.prototype.invoke = function (name /*...args*/) {
    return this.dispatch("post", [name, array_slice(arguments, 1)]);
};

/**
 * Applies the promised function in a future turn.
 * @param object    promise or immediate reference for target function
 * @param args      array of application arguments
 */
Q.fapply = function (object, args) {
    return Q(object).dispatch("apply", [void 0, args]);
};

Promise.prototype.fapply = function (args) {
    return this.dispatch("apply", [void 0, args]);
};

/**
 * Calls the promised function in a future turn.
 * @param object    promise or immediate reference for target function
 * @param ...args   array of application arguments
 */
Q["try"] =
Q.fcall = function (object /* ...args*/) {
    return Q(object).dispatch("apply", [void 0, array_slice(arguments, 1)]);
};

Promise.prototype.fcall = function (/*...args*/) {
    return this.dispatch("apply", [void 0, array_slice(arguments)]);
};

/**
 * Binds the promised function, transforming return values into a fulfilled
 * promise and thrown errors into a rejected one.
 * @param object    promise or immediate reference for target function
 * @param ...args   array of application arguments
 */
Q.fbind = function (object /*...args*/) {
    var promise = Q(object);
    var args = array_slice(arguments, 1);
    return function fbound() {
        return promise.dispatch("apply", [
            this,
            args.concat(array_slice(arguments))
        ]);
    };
};
Promise.prototype.fbind = function (/*...args*/) {
    var promise = this;
    var args = array_slice(arguments);
    return function fbound() {
        return promise.dispatch("apply", [
            this,
            args.concat(array_slice(arguments))
        ]);
    };
};

/**
 * Requests the names of the owned properties of a promised
 * object in a future turn.
 * @param object    promise or immediate reference for target object
 * @return promise for the keys of the eventually settled object
 */
Q.keys = function (object) {
    return Q(object).dispatch("keys", []);
};

Promise.prototype.keys = function () {
    return this.dispatch("keys", []);
};

/**
 * Turns an array of promises into a promise for an array.  If any of
 * the promises gets rejected, the whole array is rejected immediately.
 * @param {Array*} an array (or promise for an array) of values (or
 * promises for values)
 * @returns a promise for an array of the corresponding values
 */
// By Mark Miller
// http://wiki.ecmascript.org/doku.php?id=strawman:concurrency&rev=1308776521#allfulfilled
Q.all = all;
function all(promises) {
    return when(promises, function (promises) {
        var pendingCount = 0;
        var deferred = defer();
        array_reduce(promises, function (undefined, promise, index) {
            var snapshot;
            if (
                isPromise(promise) &&
                (snapshot = promise.inspect()).state === "fulfilled"
            ) {
                promises[index] = snapshot.value;
            } else {
                ++pendingCount;
                when(
                    promise,
                    function (value) {
                        promises[index] = value;
                        if (--pendingCount === 0) {
                            deferred.resolve(promises);
                        }
                    },
                    deferred.reject,
                    function (progress) {
                        deferred.notify({ index: index, value: progress });
                    }
                );
            }
        }, void 0);
        if (pendingCount === 0) {
            deferred.resolve(promises);
        }
        return deferred.promise;
    });
}

Promise.prototype.all = function () {
    return all(this);
};

/**
 * Returns the first resolved promise of an array. Prior rejected promises are
 * ignored.  Rejects only if all promises are rejected.
 * @param {Array*} an array containing values or promises for values
 * @returns a promise fulfilled with the value of the first resolved promise,
 * or a rejected promise if all promises are rejected.
 */
Q.any = any;

function any(promises) {
    if (promises.length === 0) {
        return Q.resolve();
    }

    var deferred = Q.defer();
    var pendingCount = 0;
    array_reduce(promises, function (prev, current, index) {
        var promise = promises[index];

        pendingCount++;

        when(promise, onFulfilled, onRejected, onProgress);
        function onFulfilled(result) {
            deferred.resolve(result);
        }
        function onRejected() {
            pendingCount--;
            if (pendingCount === 0) {
                deferred.reject(new Error(
                    "Can't get fulfillment value from any promise, all " +
                    "promises were rejected."
                ));
            }
        }
        function onProgress(progress) {
            deferred.notify({
                index: index,
                value: progress
            });
        }
    }, undefined);

    return deferred.promise;
}

Promise.prototype.any = function () {
    return any(this);
};

/**
 * Waits for all promises to be settled, either fulfilled or
 * rejected.  This is distinct from `all` since that would stop
 * waiting at the first rejection.  The promise returned by
 * `allResolved` will never be rejected.
 * @param promises a promise for an array (or an array) of promises
 * (or values)
 * @return a promise for an array of promises
 */
Q.allResolved = deprecate(allResolved, "allResolved", "allSettled");
function allResolved(promises) {
    return when(promises, function (promises) {
        promises = array_map(promises, Q);
        return when(all(array_map(promises, function (promise) {
            return when(promise, noop, noop);
        })), function () {
            return promises;
        });
    });
}

Promise.prototype.allResolved = function () {
    return allResolved(this);
};

/**
 * @see Promise#allSettled
 */
Q.allSettled = allSettled;
function allSettled(promises) {
    return Q(promises).allSettled();
}

/**
 * Turns an array of promises into a promise for an array of their states (as
 * returned by `inspect`) when they have all settled.
 * @param {Array[Any*]} values an array (or promise for an array) of values (or
 * promises for values)
 * @returns {Array[State]} an array of states for the respective values.
 */
Promise.prototype.allSettled = function () {
    return this.then(function (promises) {
        return all(array_map(promises, function (promise) {
            promise = Q(promise);
            function regardless() {
                return promise.inspect();
            }
            return promise.then(regardless, regardless);
        }));
    });
};

/**
 * Captures the failure of a promise, giving an oportunity to recover
 * with a callback.  If the given promise is fulfilled, the returned
 * promise is fulfilled.
 * @param {Any*} promise for something
 * @param {Function} callback to fulfill the returned promise if the
 * given promise is rejected
 * @returns a promise for the return value of the callback
 */
Q.fail = // XXX legacy
Q["catch"] = function (object, rejected) {
    return Q(object).then(void 0, rejected);
};

Promise.prototype.fail = // XXX legacy
Promise.prototype["catch"] = function (rejected) {
    return this.then(void 0, rejected);
};

/**
 * Attaches a listener that can respond to progress notifications from a
 * promise's originating deferred. This listener receives the exact arguments
 * passed to ``deferred.notify``.
 * @param {Any*} promise for something
 * @param {Function} callback to receive any progress notifications
 * @returns the given promise, unchanged
 */
Q.progress = progress;
function progress(object, progressed) {
    return Q(object).then(void 0, void 0, progressed);
}

Promise.prototype.progress = function (progressed) {
    return this.then(void 0, void 0, progressed);
};

/**
 * Provides an opportunity to observe the settling of a promise,
 * regardless of whether the promise is fulfilled or rejected.  Forwards
 * the resolution to the returned promise when the callback is done.
 * The callback can return a promise to defer completion.
 * @param {Any*} promise
 * @param {Function} callback to observe the resolution of the given
 * promise, takes no arguments.
 * @returns a promise for the resolution of the given promise when
 * ``fin`` is done.
 */
Q.fin = // XXX legacy
Q["finally"] = function (object, callback) {
    return Q(object)["finally"](callback);
};

Promise.prototype.fin = // XXX legacy
Promise.prototype["finally"] = function (callback) {
    callback = Q(callback);
    return this.then(function (value) {
        return callback.fcall().then(function () {
            return value;
        });
    }, function (reason) {
        // TODO attempt to recycle the rejection with "this".
        return callback.fcall().then(function () {
            throw reason;
        });
    });
};

/**
 * Terminates a chain of promises, forcing rejections to be
 * thrown as exceptions.
 * @param {Any*} promise at the end of a chain of promises
 * @returns nothing
 */
Q.done = function (object, fulfilled, rejected, progress) {
    return Q(object).done(fulfilled, rejected, progress);
};

Promise.prototype.done = function (fulfilled, rejected, progress) {
    var onUnhandledError = function (error) {
        // forward to a future turn so that ``when``
        // does not catch it and turn it into a rejection.
        Q.nextTick(function () {
            makeStackTraceLong(error, promise);
            if (Q.onerror) {
                Q.onerror(error);
            } else {
                throw error;
            }
        });
    };

    // Avoid unnecessary `nextTick`ing via an unnecessary `when`.
    var promise = fulfilled || rejected || progress ?
        this.then(fulfilled, rejected, progress) :
        this;

    if (typeof process === "object" && process && process.domain) {
        onUnhandledError = process.domain.bind(onUnhandledError);
    }

    promise.then(void 0, onUnhandledError);
};

/**
 * Causes a promise to be rejected if it does not get fulfilled before
 * some milliseconds time out.
 * @param {Any*} promise
 * @param {Number} milliseconds timeout
 * @param {Any*} custom error message or Error object (optional)
 * @returns a promise for the resolution of the given promise if it is
 * fulfilled before the timeout, otherwise rejected.
 */
Q.timeout = function (object, ms, error) {
    return Q(object).timeout(ms, error);
};

Promise.prototype.timeout = function (ms, error) {
    var deferred = defer();
    var timeoutId = setTimeout(function () {
        if (!error || "string" === typeof error) {
            error = new Error(error || "Timed out after " + ms + " ms");
            error.code = "ETIMEDOUT";
        }
        deferred.reject(error);
    }, ms);

    this.then(function (value) {
        clearTimeout(timeoutId);
        deferred.resolve(value);
    }, function (exception) {
        clearTimeout(timeoutId);
        deferred.reject(exception);
    }, deferred.notify);

    return deferred.promise;
};

/**
 * Returns a promise for the given value (or promised value), some
 * milliseconds after it resolved. Passes rejections immediately.
 * @param {Any*} promise
 * @param {Number} milliseconds
 * @returns a promise for the resolution of the given promise after milliseconds
 * time has elapsed since the resolution of the given promise.
 * If the given promise rejects, that is passed immediately.
 */
Q.delay = function (object, timeout) {
    if (timeout === void 0) {
        timeout = object;
        object = void 0;
    }
    return Q(object).delay(timeout);
};

Promise.prototype.delay = function (timeout) {
    return this.then(function (value) {
        var deferred = defer();
        setTimeout(function () {
            deferred.resolve(value);
        }, timeout);
        return deferred.promise;
    });
};

/**
 * Passes a continuation to a Node function, which is called with the given
 * arguments provided as an array, and returns a promise.
 *
 *      Q.nfapply(FS.readFile, [__filename])
 *      .then(function (content) {
 *      })
 *
 */
Q.nfapply = function (callback, args) {
    return Q(callback).nfapply(args);
};

Promise.prototype.nfapply = function (args) {
    var deferred = defer();
    var nodeArgs = array_slice(args);
    nodeArgs.push(deferred.makeNodeResolver());
    this.fapply(nodeArgs).fail(deferred.reject);
    return deferred.promise;
};

/**
 * Passes a continuation to a Node function, which is called with the given
 * arguments provided individually, and returns a promise.
 * @example
 * Q.nfcall(FS.readFile, __filename)
 * .then(function (content) {
 * })
 *
 */
Q.nfcall = function (callback /*...args*/) {
    var args = array_slice(arguments, 1);
    return Q(callback).nfapply(args);
};

Promise.prototype.nfcall = function (/*...args*/) {
    var nodeArgs = array_slice(arguments);
    var deferred = defer();
    nodeArgs.push(deferred.makeNodeResolver());
    this.fapply(nodeArgs).fail(deferred.reject);
    return deferred.promise;
};

/**
 * Wraps a NodeJS continuation passing function and returns an equivalent
 * version that returns a promise.
 * @example
 * Q.nfbind(FS.readFile, __filename)("utf-8")
 * .then(console.log)
 * .done()
 */
Q.nfbind =
Q.denodeify = function (callback /*...args*/) {
    var baseArgs = array_slice(arguments, 1);
    return function () {
        var nodeArgs = baseArgs.concat(array_slice(arguments));
        var deferred = defer();
        nodeArgs.push(deferred.makeNodeResolver());
        Q(callback).fapply(nodeArgs).fail(deferred.reject);
        return deferred.promise;
    };
};

Promise.prototype.nfbind =
Promise.prototype.denodeify = function (/*...args*/) {
    var args = array_slice(arguments);
    args.unshift(this);
    return Q.denodeify.apply(void 0, args);
};

Q.nbind = function (callback, thisp /*...args*/) {
    var baseArgs = array_slice(arguments, 2);
    return function () {
        var nodeArgs = baseArgs.concat(array_slice(arguments));
        var deferred = defer();
        nodeArgs.push(deferred.makeNodeResolver());
        function bound() {
            return callback.apply(thisp, arguments);
        }
        Q(bound).fapply(nodeArgs).fail(deferred.reject);
        return deferred.promise;
    };
};

Promise.prototype.nbind = function (/*thisp, ...args*/) {
    var args = array_slice(arguments, 0);
    args.unshift(this);
    return Q.nbind.apply(void 0, args);
};

/**
 * Calls a method of a Node-style object that accepts a Node-style
 * callback with a given array of arguments, plus a provided callback.
 * @param object an object that has the named method
 * @param {String} name name of the method of object
 * @param {Array} args arguments to pass to the method; the callback
 * will be provided by Q and appended to these arguments.
 * @returns a promise for the value or error
 */
Q.nmapply = // XXX As proposed by "Redsandro"
Q.npost = function (object, name, args) {
    return Q(object).npost(name, args);
};

Promise.prototype.nmapply = // XXX As proposed by "Redsandro"
Promise.prototype.npost = function (name, args) {
    var nodeArgs = array_slice(args || []);
    var deferred = defer();
    nodeArgs.push(deferred.makeNodeResolver());
    this.dispatch("post", [name, nodeArgs]).fail(deferred.reject);
    return deferred.promise;
};

/**
 * Calls a method of a Node-style object that accepts a Node-style
 * callback, forwarding the given variadic arguments, plus a provided
 * callback argument.
 * @param object an object that has the named method
 * @param {String} name name of the method of object
 * @param ...args arguments to pass to the method; the callback will
 * be provided by Q and appended to these arguments.
 * @returns a promise for the value or error
 */
Q.nsend = // XXX Based on Mark Miller's proposed "send"
Q.nmcall = // XXX Based on "Redsandro's" proposal
Q.ninvoke = function (object, name /*...args*/) {
    var nodeArgs = array_slice(arguments, 2);
    var deferred = defer();
    nodeArgs.push(deferred.makeNodeResolver());
    Q(object).dispatch("post", [name, nodeArgs]).fail(deferred.reject);
    return deferred.promise;
};

Promise.prototype.nsend = // XXX Based on Mark Miller's proposed "send"
Promise.prototype.nmcall = // XXX Based on "Redsandro's" proposal
Promise.prototype.ninvoke = function (name /*...args*/) {
    var nodeArgs = array_slice(arguments, 1);
    var deferred = defer();
    nodeArgs.push(deferred.makeNodeResolver());
    this.dispatch("post", [name, nodeArgs]).fail(deferred.reject);
    return deferred.promise;
};

/**
 * If a function would like to support both Node continuation-passing-style and
 * promise-returning-style, it can end its internal promise chain with
 * `nodeify(nodeback)`, forwarding the optional nodeback argument.  If the user
 * elects to use a nodeback, the result will be sent there.  If they do not
 * pass a nodeback, they will receive the result promise.
 * @param object a result (or a promise for a result)
 * @param {Function} nodeback a Node.js-style callback
 * @returns either the promise or nothing
 */
Q.nodeify = nodeify;
function nodeify(object, nodeback) {
    return Q(object).nodeify(nodeback);
}

Promise.prototype.nodeify = function (nodeback) {
    if (nodeback) {
        this.then(function (value) {
            Q.nextTick(function () {
                nodeback(null, value);
            });
        }, function (error) {
            Q.nextTick(function () {
                nodeback(error);
            });
        });
    } else {
        return this;
    }
};

Q.noConflict = function() {
    throw new Error("Q.noConflict only works when Q is used as a global");
};

// All code before this point will be filtered from stack traces.
var qEndingLine = captureLine();

return Q;

});

}).call(this,require("timers").setImmediate)

},{"timers":undefined}],19:[function(require,module,exports){
/*!---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
"use strict";
var __extends = (this && this.__extends) || function (d, b) {
    for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
    function __() { this.constructor = d; }
    d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
};
var _1 = require("./");
var path = require("path");
;
var ErrorInformation = (function () {
    function ErrorInformation(options) {
        if (!options.rawError) {
            throw new Error("You must provide an error to process");
        }
        this._rawError = options.rawError;
        this._errorType = options.errorType;
        this._packageInfo = options.packageInfo;
        this._exeName = defaultValue(path.basename(process.execPath), options.exeName);
        this._errorReporter = options.errorReporter;
        this._pathMode = defaultValue(_1.pathMode.defaultMode, options.mode);
    }
    ErrorInformation.prototype.getStructuredStack = function () {
        if (!this._structuredError) {
            try {
                this._structuredError = this.rawToStructured(this._rawError.stack);
            }
            catch (e) {
                if (this.errorReporter) {
                    this.errorReporter.emitInternalError(e, this._rawError, "");
                }
                throw e;
            }
        }
        return this._structuredError;
    };
    ErrorInformation.prototype.getStandardStack = function () {
        if (!this._standardStack) {
            try {
                this._standardStack = _1.structuredToStandard(this.getStructuredStack());
            }
            catch (e) {
                if (this.errorReporter) {
                    this.errorReporter.emitInternalError(e, this._rawError, "");
                }
                throw e;
            }
        }
        return this._standardStack;
    };
    ErrorInformation.prototype.getVerboseStack = function () {
        if (!this._verboseStack) {
            try {
                this._verboseStack = _1.structuredToSemiWatson(this.getStructuredStack());
            }
            catch (e) {
                if (this.errorReporter) {
                    this.errorReporter.emitInternalError(e, this._rawError, "");
                }
                throw e;
            }
        }
        return this._verboseStack;
    };
    Object.defineProperty(ErrorInformation.prototype, "errorName", {
        get: function () {
            return this._rawError.name;
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(ErrorInformation.prototype, "errorMessage", {
        get: function () {
            return this._rawError.message;
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(ErrorInformation.prototype, "errorType", {
        get: function () {
            return this._errorType;
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(ErrorInformation.prototype, "rawError", {
        get: function () {
            return this._rawError;
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(ErrorInformation.prototype, "packageInfo", {
        get: function () {
            return this._packageInfo;
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(ErrorInformation.prototype, "exeName", {
        get: function () {
            return this._exeName;
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(ErrorInformation.prototype, "errorReporter", {
        get: function () {
            return this._errorReporter;
        },
        enumerable: true,
        configurable: true
    });
    ErrorInformation.prototype.rawToStructured = function (raw) {
        var stackProc = new _1.StackProcessor({
            appRoot: this.packageInfo.packageJsonFullPath,
            errorReporter: this.errorReporter,
            packageInfo: this.packageInfo,
            mode: this._pathMode });
        var originalLines = raw.split("\n");
        var structuredStack;
        try {
            structuredStack = stackProc.parseFirstLine(originalLines[0]);
        }
        catch (e) {
            structuredStack = { ErrorType: this.errorName, ErrorMessage: this.errorMessage, Stack: [] };
        }
        for (var i = 1; i < originalLines.length; i++) {
            var currentFrame = void 0;
            try {
                currentFrame = stackProc.parseStackLine(originalLines[i], true);
            }
            catch (e) {
                if (this.errorReporter) {
                    this.errorReporter.emitInternalError(e, this.rawError, originalLines[i]);
                }
                currentFrame = {
                    RelativePath: "UNSUPPORTED",
                    extra: {
                        failedToParse: true,
                        rawString: originalLines[i]
                    }
                };
            }
            structuredStack.Stack.push(currentFrame);
        }
        return structuredStack;
    };
    return ErrorInformation;
}());
exports.ErrorInformation = ErrorInformation;
function setStackTraceLimit(limit) {
    try {
        Error.stackTraceLimit(limit);
    }
    catch (e) {
        var err = Error;
        err.stackTraceLimit = limit;
    }
}
exports.setStackTraceLimit = setStackTraceLimit;
function coerceToError(e) {
    if (e instanceof Error) {
        return e;
    }
    return new NonError(e);
}
exports.coerceToError = coerceToError;
var NonError = (function (_super) {
    __extends(NonError, _super);
    function NonError(input) {
        _super.call(this);
        this.name = "NonError";
        if (input.message) {
            this.message = input.message;
        }
        else {
            this.message = JSON.stringify(input);
        }
        if (input.stack) {
            this.stack = input.stack;
        }
        else {
            var header = this.name + ": " + this.message;
            this.stack = header + "\nat NonError";
        }
        this.originalObject = input;
    }
    return NonError;
}(Error));
exports.NonError = NonError;
function defaultValue(defaultValue, actual, strict) {
    if (strict === void 0) { strict = true; }
    if ((strict && actual === undefined) || (!strict && !actual)) {
        return defaultValue;
    }
    return actual;
}
exports.defaultValue = defaultValue;

},{"./":20,"path":undefined}],20:[function(require,module,exports){
/*!---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
"use strict";
function __export(m) {
    for (var p in m) if (!exports.hasOwnProperty(p)) exports[p] = m[p];
}
__export(require("./error-info"));
__export(require("./processor"));
__export(require("./utils"));
var reporting_channel_1 = require("../reporting-channel");
exports.ReportingChannel = reporting_channel_1.ReportingChannel;
__export(require("../error-reporter-base"));

},{"../error-reporter-base":24,"../reporting-channel":26,"./error-info":19,"./processor":21,"./utils":22}],21:[function(require,module,exports){
/*!---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
"use strict";
var _1 = require("./");
var path_1 = require("path");
var defaultpath = require("path");
var fs = require("fs");
var NODE_MODULES_DIRNAME = "node_modules";
var PACKAGE_JSON_NAME = "package.json";
var VALID_NON_CODE_LOCATIONS = ["native", "NonError", "unknown"];
var StackProcessor = (function () {
    function StackProcessor(options) {
        this.doFileIO = true;
        this.appRoot = options.appRoot;
        this._errorReporter = options.errorReporter;
        this._packageInfo = options.packageInfo;
        switch (options.mode) {
            case _1.pathMode.defaultMode:
                this.path = defaultpath;
                break;
            case _1.pathMode.posix:
                this.path = path_1.posix;
                break;
            case _1.pathMode.windows:
                this.path = path_1.win32;
                break;
            default:
                this.path = defaultpath;
        }
    }
    StackProcessor.prototype.parseFirstLine = function (firstLine) {
        var firstLineRX = /(^.+?\b)(?:: )?(?:(.+?\b): )?(.*)/;
        var ERROR_SS = 1;
        var SYSTEM_ERROR_SS = 2;
        var MESSAGE_SS = 3;
        var matches = firstLine.match(firstLineRX);
        var type;
        if (matches[ERROR_SS] === "Error" && matches[SYSTEM_ERROR_SS]) {
            type = matches[SYSTEM_ERROR_SS];
        }
        else {
            type = matches[ERROR_SS];
        }
        var message = matches[MESSAGE_SS];
        return { ErrorType: type, ErrorMessage: message, Stack: [] };
    };
    StackProcessor.prototype.parseStackLine = function (inputLine, getPackageInfo) {
        if (getPackageInfo === void 0) { getPackageInfo = false; }
        var isEval = false;
        if (inputLine.includes("eval at")) {
            isEval = true;
            inputLine = this.cleanEvalFrames(inputLine);
        }
        inputLine = _1.replaceAll(inputLine, "(anonymous function)", "(anonymousfunction)");
        inputLine = _1.replaceAll(inputLine, "unknown location", "unknown");
        var mainRX = /\s*at (new )?(.+?)(?: \[as ([^ ]+?)\])?(?: \((.*)\))?$/;
        var newSS = 1;
        var functionNameSS = 2;
        var asMethodSS = 3;
        var locationSS = 4;
        var functionName;
        var location;
        var atNew = false;
        var asString = "";
        var mainMatches = inputLine.match(mainRX);
        if (!mainMatches) {
            var e = new Error("unrecognized stack trace format: " + inputLine);
            if (this._errorReporter) {
                this._errorReporter.emitInternalError(e, null, inputLine);
            }
            var temp = {
                RelativePath: "UNSUPPORTED",
                Package: { name: "UNSUPPORTED" },
                line: -1,
                column: -1,
                extra: { failedToParse: true } };
            return temp;
        }
        if (mainMatches[newSS]) {
            atNew = true;
        }
        if (mainMatches[asMethodSS]) {
            asString = mainMatches[asMethodSS];
        }
        if (mainMatches[locationSS]) {
            location = mainMatches[locationSS];
            functionName = mainMatches[functionNameSS];
        }
        else {
            location = mainMatches[functionNameSS];
            functionName = "";
        }
        var result = this.parseCodeLocation(location);
        result.FunctionName = functionName;
        var stripped = this.getPackageInfo(result.RelativePath);
        result.RelativePath = stripped.strippedPath;
        if (getPackageInfo && stripped.package) {
            result.Package = stripped.package;
        }
        if (atNew || asString || isEval) {
            if (atNew) {
                result.FunctionName += ".constructor";
            }
            if (asString) {
                result.FunctionName += "[" + asString + "]";
            }
            result.extra = { isNew: atNew, hasAs: asString, isEval: isEval, failedToParse: false, rawString: "" };
        }
        return result;
    };
    StackProcessor.prototype.cleanEvalFrames = function (input) {
        var evalRX = /\eval at .+\), /;
        return input.replace(evalRX, "");
    };
    StackProcessor.prototype.parseCodeLocation = function (input) {
        var i = VALID_NON_CODE_LOCATIONS.length;
        var element;
        while (i--) {
            element = VALID_NON_CODE_LOCATIONS[i];
            if (element === input) {
                return { RelativePath: input, line: -1, column: -1, FunctionName: "" };
            }
        }
        input = _1.replaceAll(input, "file:///", "");
        input = _1.replaceAll(input, "%20", " ");
        var locationRX = /(?:(^.+):(\d+):(\d+))/;
        var PATH_SS = 1;
        var LINE_SS = 2;
        var COLUMN_SS = 3;
        var line;
        var column;
        var matches = input.match(locationRX);
        if (!matches) {
            var intError = new Error("unsupported location format");
            if (this._errorReporter) {
                this._errorReporter.emitInternalError(intError, null, input);
            }
            return { RelativePath: "UNSUPPORTED", line: -1, column: -1, FunctionName: "" };
        }
        else {
            line = parseInt(matches[LINE_SS], 10);
            column = parseInt(matches[COLUMN_SS], 10);
        }
        var path = this.path.normalize(matches[PATH_SS]);
        return { RelativePath: path, line: line, column: column, FunctionName: "" };
    };
    StackProcessor.prototype.getPackageInfo = function (filePath) {
        var i = VALID_NON_CODE_LOCATIONS.length;
        var element;
        while (i--) {
            element = VALID_NON_CODE_LOCATIONS[i];
            if (element === filePath) {
                return { package: { name: "UNKNOWN" }, strippedPath: filePath };
            }
        }
        if (!this.path.isAbsolute(filePath)) {
            return {
                package: { name: "node", version: process.version },
                strippedPath: filePath };
        }
        var relPath = this.path.relative(this.appRoot, filePath);
        var nodeModIndex = filePath.lastIndexOf(NODE_MODULES_DIRNAME);
        if (nodeModIndex !== -1) {
            var modRoot = filePath.substring(0, nodeModIndex + NODE_MODULES_DIRNAME.length + 1);
            var modPath = this.path.relative(modRoot, filePath);
            var sepIndex = modPath.indexOf(this.path.sep);
            if (sepIndex !== -1) {
                var modName = modPath.substring(0, sepIndex);
                relPath = modPath.substring(sepIndex + 1);
                var pkg = { name: modName };
                if (this.doFileIO) {
                    try {
                        var fullPath = this.path.join(modRoot, modName, PACKAGE_JSON_NAME);
                        var pkgjs = JSON.parse(fs.readFileSync(fullPath, "utf8"));
                        pkg.packageJsonFullPath = fullPath;
                        if (pkgjs.name) {
                            pkg.name = pkgjs.name;
                        }
                        if (pkgjs.version) {
                            pkg.version = pkgjs.version;
                        }
                    }
                    catch (e) {
                    }
                }
                relPath = _1.convertPathToPosix(relPath);
                return { package: pkg, strippedPath: relPath };
            }
            if (this._errorReporter) {
                this._errorReporter.emitInternalError(new Error("javascript file in unexpected location"), null, filePath);
            }
            filePath = this.path.basename(filePath);
            return { package: { name: "UNKNOWN" }, strippedPath: filePath };
        }
        if (filePath.includes(".asar")) {
            var index = filePath.lastIndexOf(".asar") + ".asar".length;
            var asarName = this.path.basename(filePath.substring(0, index));
            var relativePath = _1.convertPathToPosix(filePath.substring(index + 1));
            relPath = _1.convertPathToPosix(relPath);
            return { package: { name: asarName }, strippedPath: relativePath };
        }
        if (relPath === filePath || relPath.startsWith("..") || this.appRoot === "") {
            filePath = this.path.basename(filePath);
            return { package: { name: "UNKNOWN" }, strippedPath: filePath };
        }
        relPath = _1.convertPathToPosix(relPath);
        return { package: this._packageInfo, strippedPath: relPath };
    };
    return StackProcessor;
}());
exports.StackProcessor = StackProcessor;
function removeUserName(input) {
    var userNameRX = /[a-z]:\\Users\\([^\\]+)/i;
    var match = input.match(userNameRX);
    if (match) {
        input = _1.replaceAll(input, match[1], "USERNAME");
    }
    return input;
}
exports.removeUserName = removeUserName;
function structuredToSemiWatson(input) {
    var _this = this;
    var WLB = _1.WATSON_LINE_BREAK;
    var result = "";
    input.Stack.forEach(function (frame) {
        result += (result ? WLB : "") + _this.frameToString(frame);
    });
    return result;
}
exports.structuredToSemiWatson = structuredToSemiWatson;
function structuredToStandard(input) {
    var stack = "";
    input.Stack.forEach(function (frame) {
        if (frame.FunctionName) {
            stack += (stack ? "\n" : "") + ("" + ((frame.extra && frame.extra.isEval) ? "eval " : ""))
                + ("at " + frame.FunctionName + " ") +
                ("(" + frame.RelativePath);
            if (frame.line !== -1 && frame.column !== -1) {
                stack += ":" + frame.line + ":" + frame.column;
            }
            stack += ")";
        }
        else {
            stack += (stack ? "\n" : "") + (((frame.extra && frame.extra.isEval) ? "eval " : "") + "at ") +
                ("" + frame.RelativePath);
            if (frame.line !== -1 && frame.column !== -1) {
                stack += ":" + frame.line + ":" + frame.column;
            }
        }
    });
    return stack;
}
exports.structuredToStandard = structuredToStandard;
function frameToString(frame) {
    if (!frame) {
        return "UNSUPPORTED\tUNSUPPORTED";
    }
    var packageString = frame.Package.name;
    if (!packageString) {
        packageString = "UNKNOWN";
    }
    if (frame.Package.version) {
        packageString += "@" + frame.Package.version;
    }
    var fileInfo = frame.RelativePath;
    if (!fileInfo) {
        fileInfo = "UNKNOWN";
    }
    if (frame.line !== -1 && frame.column !== -1) {
        fileInfo += ":" + frame.line + ":" + frame.column;
    }
    var result = packageString + "\t" + fileInfo;
    if (frame.FunctionName) {
        result += "\t" + frame.FunctionName;
    }
    return result;
}
exports.frameToString = frameToString;

},{"./":20,"fs":undefined,"path":undefined}],22:[function(require,module,exports){
/*!---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
"use strict";
var path_1 = require("path");
exports.WATSON_LINE_BREAK = "\r\n";
(function (pathMode) {
    pathMode[pathMode["windows"] = 0] = "windows";
    pathMode[pathMode["posix"] = 1] = "posix";
    pathMode[pathMode["defaultMode"] = 2] = "defaultMode";
})(exports.pathMode || (exports.pathMode = {}));
var pathMode = exports.pathMode;
function convertPathToPosix(input) {
    input = replaceAll(input, "\\", "/");
    var networkPath = input.startsWith("//");
    if (!networkPath) {
        var driveLetterRX = /^([a-zA-Z]):/;
        input = input.replace(driveLetterRX, "/$1");
    }
    input = path_1.posix.normalize(input);
    if (networkPath) {
        input = "/" + input;
    }
    return input;
}
exports.convertPathToPosix = convertPathToPosix;
function convertPathToWindows(input) {
    input = replaceAll(input, "/", "\\");
    var networkPath = input.startsWith("\\\\");
    if (input.startsWith("\\") && !networkPath) {
        var driveLetterRX = /^\\([a-zA-Z])\\/;
        var match = input.match(driveLetterRX);
        if (match) {
            input = input.replace(driveLetterRX, match[1] + ":\\");
        }
        else {
            input = "A:" + input;
        }
    }
    input = path_1.win32.normalize(input);
    return input;
}
exports.convertPathToWindows = convertPathToWindows;
function replaceAll(input, find, replace) {
    if (!find) {
        return input;
    }
    if (find === replace) {
        return input;
    }
    if (replace.includes(find)) {
        throw new Error("replacement string includes the search string. This will send you into an infinite loop.");
    }
    while (input.includes(find)) {
        input = input.replace(find, replace);
    }
    return input;
}
exports.replaceAll = replaceAll;
var SEM_VER = /^(\d+\.\d+\.\d+(?:\.\d+)?)(?:-([A-Za-z0-9\.\-]+))?(?:\+([A-Za-z0-9\.\-]+))?$/;
var MAIN = 1;
var TAG = 2;
var DATE_YYYYMMDD = /\d{4}(\d{4})/;
function convertVersion(input) {
    var match = input.match(SEM_VER);
    if (match) {
        var release = match[MAIN];
        if (release.split(".").length === 4) {
            return release;
        }
        var finalNumber = ".";
        var tag = match[TAG];
        if (tag) {
            var tagComponents = tag.split(".");
            if (tagComponents.length !== 0) {
                var date = tagComponents[0].match(DATE_YYYYMMDD);
                if (date) {
                    finalNumber += date[1];
                }
                var lastIndex = tagComponents.length - 1;
                if (!Number.isNaN(Number.parseInt(tagComponents[lastIndex]))) {
                    finalNumber += tagComponents[lastIndex];
                }
            }
        }
        if (finalNumber !== ".") {
            return release + finalNumber;
        }
        else {
            return release;
        }
    }
    else {
        return replaceAll(input, "_", "-");
    }
}
exports.convertVersion = convertVersion;

},{"path":undefined}],23:[function(require,module,exports){
/*!---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
"use strict";
var __extends = (this && this.__extends) || function (d, b) {
    for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
    function __() { this.constructor = d; }
    d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
};
var error_reporter_base_1 = require("./error-reporter-base");
var advanced_1 = require("./advanced");
var try_all_1 = require("./try-all");
(function (State) {
    State[State["ready"] = 0] = "ready";
    State[State["reporting"] = 1] = "reporting";
    State[State["finishing"] = 2] = "finishing";
})(exports.State || (exports.State = {}));
var State = exports.State;
var ERROR_REPORTING_ASYNC_FAILURE = "ErrorReportAsyncFailure";
var USER_CALLBACK_UNCAUGHT_EXCEPTION = "userCallbackUncaught";
var ErrorHandlerNode = (function (_super) {
    __extends(ErrorHandlerNode, _super);
    function ErrorHandlerNode(options) {
        _super.call(this, { appRoot: options.appRoot, packageInfo: options.packageInfo, exeName: options.exeName });
        this.errorType = "NodeFatal";
        this.finalCallback = options.finalCallback;
        this.currentState = State.ready;
        advanced_1.setStackTraceLimit(Infinity);
        process.on("uncaughtException", this.handleUncaughtMain.bind(this));
    }
    ErrorHandlerNode.prototype.handleUncaughtMain = function (err) {
        var _this = this;
        if (this.currentState === State.ready) {
            this.currentState = State.reporting;
        }
        else if (this.currentState === State.reporting) {
            try {
                this.emitAsyncFailure(err);
            }
            catch (e) {
            }
            return;
        }
        else if (this.currentState === State.finishing) {
            this.finalExit();
        }
        var e = advanced_1.coerceToError(err);
        this.reportError(e, this.errorType).then(function () { _this.finishHandling(e); });
    };
    ErrorHandlerNode.prototype.finishHandling = function (err) {
        this.currentState = State.finishing;
        try {
            if (this.finalCallback && this.finalCallback(err)) {
                this.currentState = State.ready;
                return;
            }
        }
        catch (e) {
            try {
                this.emitUserCallbackUncaughtException(e);
            }
            catch (e1) {
            }
        }
        try {
            this.emitAboutToExit().then(this.finalExit, this.finalExit);
            setTimeout(this.finalExit, 500);
        }
        catch (e2) {
            this.finalExit();
        }
    };
    ErrorHandlerNode.prototype.finalExit = function () {
        if (process.platform === "win32") {
            process.abort();
        }
        process.exit(1);
    };
    ErrorHandlerNode.prototype.onAboutToExit = function (callback) {
        if (this.aboutToExitPromises === undefined) {
            this.aboutToExitPromises = [];
        }
        this.aboutToExitPromises.push(callback);
    };
    ErrorHandlerNode.prototype.emitAboutToExit = function () {
        if (this.aboutToExitPromises) {
            var promises_1 = [];
            this.aboutToExitPromises.forEach(function (element) {
                try {
                    promises_1.push(element());
                }
                catch (e) {
                }
            });
            return try_all_1.tryAll(promises_1);
        }
        return Promise.resolve();
    };
    ErrorHandlerNode.prototype.onAsyncFailure = function (callback) {
        this.emitter.on(ERROR_REPORTING_ASYNC_FAILURE, callback);
    };
    ErrorHandlerNode.prototype.emitAsyncFailure = function (asyncError) {
        return this.emitter.emit(ERROR_REPORTING_ASYNC_FAILURE, asyncError);
    };
    ErrorHandlerNode.prototype.onUserCallbackUncaughtException = function (callback) {
        this.emitter.on(USER_CALLBACK_UNCAUGHT_EXCEPTION, callback);
    };
    ErrorHandlerNode.prototype.emitUserCallbackUncaughtException = function (uncaughtException) {
        this.emitter.emit(USER_CALLBACK_UNCAUGHT_EXCEPTION, uncaughtException);
    };
    return ErrorHandlerNode;
}(error_reporter_base_1.ErrorReporter));
exports.ErrorHandlerNode = ErrorHandlerNode;

},{"./advanced":20,"./error-reporter-base":24,"./try-all":27}],24:[function(require,module,exports){
/*!---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
"use strict";
var try_all_1 = require("./try-all");
var path = require("path");
var advanced_1 = require("./advanced");
var events_1 = require("events");
var INTERNAL_ERROR = "internalError";
var ERROR_REPORTING_STARTED = "reportingStarted";
var ERROR_REPORTING_FINISHED = "reportingFinished";
var ErrorReporter = (function () {
    function ErrorReporter(config) {
        this._packageInfo = config.packageInfo;
        if (config.appRoot === "" || path.isAbsolute(config.appRoot)) {
            this._packageInfo.packageJsonFullPath = config.appRoot;
        }
        else {
            throw new Error(("Invalid app root \"" + config.appRoot + "\". ") +
                "You must provide an absolute path or an empty string");
        }
        this._exeName = config.exeName;
        this.emitter = new events_1.EventEmitter();
        this.channels = [];
    }
    ErrorReporter.prototype.addReportingChannel = function (channel) {
        this.channels.push(channel);
        channel.registerListeners(this);
    };
    ErrorReporter.prototype.removeReportingChannel = function (channel) {
        var index = this.channels.indexOf(channel);
        if (index !== -1) {
            this.channels.splice(index, 1);
        }
    };
    Object.defineProperty(ErrorReporter.prototype, "appName", {
        get: function () {
            return this._packageInfo.name;
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(ErrorReporter.prototype, "appVersion", {
        get: function () {
            return this._packageInfo.version;
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(ErrorReporter.prototype, "appRoot", {
        get: function () {
            return this._packageInfo.packageJsonFullPath;
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(ErrorReporter.prototype, "branch", {
        get: function () {
            return this._packageInfo.branch;
        },
        enumerable: true,
        configurable: true
    });
    ErrorReporter.prototype.reportError = function (err, errorType) {
        var _this = this;
        this.emitReportingStarted();
        var stack;
        try {
            stack = new advanced_1.ErrorInformation({ rawError: err,
                packageInfo: this._packageInfo,
                errorType: errorType,
                exeName: this._exeName,
                errorReporter: this });
        }
        catch (e) {
            try {
                this.emitInternalError(e, err, "");
            }
            catch (e1) {
            }
            if (!stack) {
                return Promise.resolve({});
            }
        }
        var reportPromises = [];
        var rejectionFunctions = [];
        var timeoutLimits = [];
        this.channels.forEach(function (channel) {
            var prom;
            try {
                prom = channel.report(stack);
            }
            catch (e) {
                prom = Promise.reject(e);
            }
            var timeoutPromise = new Promise(function (resolve, reject) {
                rejectionFunctions.push(reject);
                timeoutLimits.push(channel.timeout);
            });
            var compositeProm = Promise.race([prom, timeoutPromise]);
            reportPromises.push(compositeProm);
        });
        for (var i = 0; i < rejectionFunctions.length; i++) {
            if (timeoutLimits[i] >= 0) {
                setTimeout(rejectionFunctions[i], timeoutLimits[i], "timed out after " + timeoutLimits[i] + " ms");
            }
        }
        return try_all_1.tryAll(reportPromises)
            .then(function (results) {
            _this.reportInternalFailures(results, _this.channels);
            _this.emitReportingFinished(results);
            return results;
        })
            .catch(function (reason) {
            return reason;
        });
    };
    ;
    ErrorReporter.prototype.reportInternalFailures = function (results, channels) {
        for (var i = 0; i < channels.length; i++) {
            if (channels[i].reportOnFailure && !results.resolved[i]) {
                this.emitInternalError(results.failReasons[i], null, null);
            }
        }
    };
    ErrorReporter.prototype.onInternalError = function (callback) {
        this.emitter.on(INTERNAL_ERROR, callback);
    };
    ErrorReporter.prototype.emitInternalError = function (internalError, errorBeingProcessed, problemString) {
        if (this.emitter.listenerCount(INTERNAL_ERROR) > 0) {
            var proc = new advanced_1.ErrorInformation({ rawError: internalError });
            var stack = void 0;
            try {
                stack = proc.getVerboseStack();
            }
            catch (e) {
                stack = "UNKNOWN\tUNKNOWN";
            }
            var info = {
                props: {
                    InternalErrorStack: stack,
                    InternalErrorType: proc.errorName
                },
                PIIProps: {
                    InternalErrorMessage: proc.errorMessage,
                    RawInternalError: internalError,
                    OriginalError: errorBeingProcessed,
                    ProblemString: problemString
                }
            };
            this.emitter.emit(INTERNAL_ERROR, info);
        }
    };
    ErrorReporter.prototype.onReportingStarted = function (callback) {
        this.emitter.on(ERROR_REPORTING_STARTED, callback);
    };
    ErrorReporter.prototype.emitReportingStarted = function () {
        return this.emitter.emit(ERROR_REPORTING_STARTED);
    };
    ErrorReporter.prototype.onReportingFinished = function (callback) {
        this.emitter.on(ERROR_REPORTING_FINISHED, callback);
    };
    ErrorReporter.prototype.emitReportingFinished = function (results) {
        return this.emitter.emit(ERROR_REPORTING_FINISHED, results);
    };
    return ErrorReporter;
}());
exports.ErrorReporter = ErrorReporter;

},{"./advanced":20,"./try-all":27,"events":undefined,"path":undefined}],25:[function(require,module,exports){
/*!---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
"use strict";
var error_handler_node_1 = require("./error-handler-node");
exports.ErrorHandlerNode = error_handler_node_1.ErrorHandlerNode;
var reporting_channel_1 = require("./reporting-channel");
exports.ConsoleReporter = reporting_channel_1.ConsoleReporter;
exports.FileReporter = reporting_channel_1.FileReporter;

},{"./error-handler-node":23,"./reporting-channel":26}],26:[function(require,module,exports){
/*!---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
"use strict";
var __extends = (this && this.__extends) || function (d, b) {
    for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
    function __() { this.constructor = d; }
    d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
};
var advanced_1 = require("./advanced");
var fs = require("fs");
var path = require("path");
var DEFAULT_REPORT_ON_FAILURE = false;
var DEFAULT_TIMEOUT = 500;
var ReportingChannel = (function () {
    function ReportingChannel(reportOnFailure, timeoutMS) {
        this._reportOnFailure = reportOnFailure !== undefined ? reportOnFailure : DEFAULT_REPORT_ON_FAILURE;
        this._timeout = timeoutMS !== undefined ? timeoutMS : DEFAULT_TIMEOUT;
    }
    Object.defineProperty(ReportingChannel.prototype, "reportOnFailure", {
        get: function () {
            return this._reportOnFailure;
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(ReportingChannel.prototype, "timeout", {
        get: function () {
            return this._timeout;
        },
        enumerable: true,
        configurable: true
    });
    ReportingChannel.prototype.registerListeners = function (reporter) {
    };
    ;
    return ReportingChannel;
}());
exports.ReportingChannel = ReportingChannel;
var ConsoleReporter = (function (_super) {
    __extends(ConsoleReporter, _super);
    function ConsoleReporter() {
        _super.call(this);
    }
    ConsoleReporter.prototype.report = function (error) {
        return new Promise(function (resolve, reject) {
            if (error.rawError.stack.length > 2000) {
                var shortened = error.rawError.stack.substr(0, 1000);
                shortened += "\n...shortened to avoid blocking while printing...\n" +
                    "...see other reporting methods for full stack...\n";
                shortened += error.rawError.stack.substring(error.rawError.stack.length - 1000);
                console.log(shortened);
            }
            else {
                console.log(error.rawError.stack);
            }
            console.log();
            resolve();
        });
    };
    return ConsoleReporter;
}(ReportingChannel));
exports.ConsoleReporter = ConsoleReporter;
var FileReporter = (function (_super) {
    __extends(FileReporter, _super);
    function FileReporter(filePath, fileName, stripStack) {
        _super.call(this);
        this.fileName = fileName;
        this.filePath = filePath;
        this.stripStack = stripStack;
    }
    FileReporter.prototype.report = function (error) {
        var _this = this;
        return new Promise(function (resolve, reject) {
            var fullPath = path.join(_this.filePath, _this.fileName);
            var content;
            if (_this.stripStack) {
                content = error.errorName + advanced_1.WATSON_LINE_BREAK;
                content += error.errorMessage + advanced_1.WATSON_LINE_BREAK;
                content += error.getVerboseStack() + advanced_1.WATSON_LINE_BREAK + advanced_1.WATSON_LINE_BREAK;
            }
            else {
                content = error.rawError.stack + "\n";
            }
            fs.appendFile(fullPath, content, {}, function (err) { if (err) {
                reject(err.stack);
            }
            else {
                resolve(fullPath);
            } });
        });
    };
    return FileReporter;
}(ReportingChannel));
exports.FileReporter = FileReporter;

},{"./advanced":20,"fs":undefined,"path":undefined}],27:[function(require,module,exports){
/*!---------------------------------------------------------
 * Copyright (C) Microsoft Corporation. All rights reserved.
 *--------------------------------------------------------*/
"use strict";
var ResultsImpl = (function () {
    function ResultsImpl() {
        this.resolved = [];
        this.results = [];
        this.failReasons = [];
    }
    return ResultsImpl;
}());
function tryAll(input) {
    if (input.length === 0) {
        return Promise.resolve(new ResultsImpl());
    }
    var internalInstance = new ResultsImpl();
    var promisesInProgress = [];
    var _loop_1 = function(i) {
        if (input[i] && input[i].then) {
            promisesInProgress.push(input[i].then(function (value) {
                internalInstance.results[i] = value;
                internalInstance.resolved[i] = true;
                return value;
            }, function (reason) {
                internalInstance.resolved[i] = false;
                internalInstance.failReasons[i] = reason;
            }));
        }
        else {
            internalInstance.resolved[i] = false;
            internalInstance.failReasons[i] = "Invalid Promise: " + JSON.stringify(input[i]);
        }
    };
    for (var i = 0; i < input.length; i++) {
        _loop_1(i);
    }
    var internalPromise = Promise.all(promisesInProgress)
        .then(function () {
        return internalInstance;
    }, function (reason) {
        return Promise.reject(reason);
    });
    return internalPromise;
}
exports.tryAll = tryAll;

},{}],28:[function(require,module,exports){
// rmdir-recursive.js

'use strict';

var fs = require('fs');
var path = require('path');


//######################################################################
/**
 * Function: remove directory recursively (async)
 * Param   : dir: path to make directory
 *           [cb]: {optional} callback(err) function
 */
function rmdirRecursive(dir, cb) {
  // check arguments
  if (typeof dir !== 'string') {
    throw new Error('rmdirRecursive: directory path required');
  }

  if (cb !== undefined && typeof cb !== 'function') {
    throw new Error('rmdirRecursive: callback must be function');
  }

  var ctx = this, called, results;

  fs.exists(dir, function existsCallback(exists) {

    // already removed? then nothing to do
    if (!exists) return rmdirRecursiveCallback(null);

    fs.stat(dir, function statCallback(err, stat) {

      if (err) return rmdirRecursiveCallback(err);

      if (!stat.isDirectory())
        return fs.unlink(dir, rmdirRecursiveCallback);

      var files = fs.readdir(dir, readdirCallback);

    }); // fs.stat callback...

    // fs.readdir callback...
    function readdirCallback(err, files) {

      if (err) return rmdirRecursiveCallback(err);

      var n = files.length;
      if (n === 0) return fs.rmdir(dir, rmdirRecursiveCallback);

      files.forEach(function (name) {

        rmdirRecursive(path.resolve(dir, name), function (err) {

          if (err) return rmdirRecursiveCallback(err);

          if (--n === 0)
            return fs.rmdir(dir, rmdirRecursiveCallback);

        }); // rmdirRecursive

      }); // files.forEach

    } // readdirCallback

  }); // fs.exists

  // rmdirRecursiveCallback(err)
  function rmdirRecursiveCallback(err) {
    if (err && err.code === 'ENOENT') err = arguments[0] = null;

    if (!results) results = arguments;
    if (!cb || called) return;
    called = true;
    cb.apply(ctx, results);
  } // rmdirRecursiveCallback

  // return rmdirRecursiveYieldable
  return function rmdirRecursiveYieldable(fn) {
    if (!cb) cb = fn;
    if (!results || called) return;
    called = true;
    cb.apply(ctx, results);
  }; // rmdirRecursiveYieldable

} // rmdirRecursive


//######################################################################
/**
 * Function: remove directory recursively (sync)
 * Param   : dir: path to remove directory
 */
function rmdirRecursiveSync(dir) {
  // check arguments
  if (typeof dir !== 'string')
    throw new Error('rmdirRecursiveSync: directory path required');

  // already removed? then nothing to do
  if (!fs.existsSync(dir)) return;

  // is file? is not directory? then remove file
  try {
    var stat = fs.statSync(dir);
  } catch (err) {
    if (err.code === 'ENOENT') return;
    throw err;
  }
  if (!stat.isDirectory()) {
    try {
      return fs.unlinkSync(dir);
    } catch (err) {
      if (err.code === 'ENOENT') return;
      throw err;
    }
  }

  // remove all contents in it
  fs.readdirSync(dir).forEach(function (name) {
    rmdirRecursiveSync(path.resolve(dir, name));
  });

  try {
    return fs.rmdirSync(dir);
  } catch (err) {
    if (err.code === 'ENOENT') return;
    throw err;
  }
}


exports = module.exports   = rmdirRecursive;
exports.rmdirRecursive     = rmdirRecursive;
exports.rmdirRecursiveSync = rmdirRecursiveSync;
exports.sync               = rmdirRecursiveSync;

},{"fs":undefined,"path":undefined}],29:[function(require,module,exports){
/*!
 * Tmp
 *
 * Copyright (c) 2011-2015 KARASZI Istvan <github@spam.raszi.hu>
 *
 * MIT Licensed
 */

/**
 * Module dependencies.
 */
var
  fs     = require('fs'),
  path   = require('path'),
  os     = require('os'),
  crypto = require('crypto'),
  exists = fs.exists || path.exists,
  existsSync = fs.existsSync || path.existsSync,
  tmpDir = require('os-tmpdir'),
  _c     = require('constants');


/**
 * The working inner variables.
 */
var
  // store the actual TMP directory
  _TMP = tmpDir(),

  // the random characters to choose from
  RANDOM_CHARS = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz',

  TEMPLATE_PATTERN = /XXXXXX/,

  DEFAULT_TRIES = 3,

  CREATE_FLAGS = _c.O_CREAT | _c.O_EXCL | _c.O_RDWR,

  DIR_MODE = 448 /* 0700 */,
  FILE_MODE = 384 /* 0600 */,

  // this will hold the objects need to be removed on exit
  _removeObjects = [],

  _gracefulCleanup = false,
  _uncaughtException = false;

/**
 * Random name generator based on crypto.
 * Adapted from http://blog.tompawlak.org/how-to-generate-random-values-nodejs-javascript
 *
 * @param {Number} howMany
 * @return {String}
 * @api private
 */
function _randomChars(howMany) {
  var
    value = [],
    rnd = null;

  // make sure that we do not fail because we ran out of entropy
  try {
    rnd = crypto.randomBytes(howMany);
  } catch (e) {
    rnd = crypto.pseudoRandomBytes(howMany);
  }

  for (var i = 0; i < howMany; i++) {
    value.push(RANDOM_CHARS[rnd[i] % RANDOM_CHARS.length]);
  }

  return value.join('');
}

/**
 * Checks whether the `obj` parameter is defined or not.
 *
 * @param {Object} obj
 * @return {Boolean}
 * @api private
 */
function _isUndefined(obj) {
  return typeof obj === 'undefined';
}

/**
 * Parses the function arguments.
 *
 * This function helps to have optional arguments.
 *
 * @param {Object} options
 * @param {Function} callback
 * @api private
 */
function _parseArguments(options, callback) {
  if (typeof options == 'function') {
    var
      tmp = options;
      options = callback || {};
      callback = tmp;
  } else if (typeof options == 'undefined') {
    options = {};
  }

  return [options, callback];
}

/**
 * Generates a new temporary name.
 *
 * @param {Object} opts
 * @returns {String}
 * @api private
 */
function _generateTmpName(opts) {
  if (opts.name) {
    return path.join(opts.dir || _TMP, opts.name);
  }

  // mkstemps like template
  if (opts.template) {
    return opts.template.replace(TEMPLATE_PATTERN, _randomChars(6));
  }

  // prefix and postfix
  var name = [
    opts.prefix || 'tmp-',
    process.pid,
    _randomChars(12),
    opts.postfix || ''
  ].join('');

  return path.join(opts.dir || _TMP, name);
}

/**
 * Gets a temporary file name.
 *
 * @param {Object} options
 * @param {Function} callback
 * @api private
 */
function _getTmpName(options, callback) {
  var
    args = _parseArguments(options, callback),
    opts = args[0],
    cb = args[1],
    tries = opts.tries || DEFAULT_TRIES;

  if (isNaN(tries) || tries < 0)
    return cb(new Error('Invalid tries'));

  if (opts.template && !opts.template.match(TEMPLATE_PATTERN))
    return cb(new Error('Invalid template provided'));

  (function _getUniqueName() {
    var name = _generateTmpName(opts);

    // check whether the path exists then retry if needed
    exists(name, function _pathExists(pathExists) {
      if (pathExists) {
        if (tries-- > 0) return _getUniqueName();

        return cb(new Error('Could not get a unique tmp filename, max tries reached ' + name));
      }

      cb(null, name);
    });
  }());
}

/**
 * Synchronous version of _getTmpName.
 *
 * @param {Object} options
 * @returns {String}
 * @api private
 */
function _getTmpNameSync(options) {
  var
    args = _parseArguments(options),
    opts = args[0],
    tries = opts.tries || DEFAULT_TRIES;

  if (isNaN(tries) || tries < 0)
    throw new Error('Invalid tries');

  if (opts.template && !opts.template.match(TEMPLATE_PATTERN))
    throw new Error('Invalid template provided');

  do {
    var name = _generateTmpName(opts);
    if (!existsSync(name)) {
      return name;
    }
  } while (tries-- > 0);

  throw new Error('Could not get a unique tmp filename, max tries reached');
}

/**
 * Creates and opens a temporary file.
 *
 * @param {Object} options
 * @param {Function} callback
 * @api public
 */
function _createTmpFile(options, callback) {
  var
    args = _parseArguments(options, callback),
    opts = args[0],
    cb = args[1];

    opts.postfix = (_isUndefined(opts.postfix)) ? '.tmp' : opts.postfix;

  // gets a temporary filename
  _getTmpName(opts, function _tmpNameCreated(err, name) {
    if (err) return cb(err);

    // create and open the file
    fs.open(name, CREATE_FLAGS, opts.mode || FILE_MODE, function _fileCreated(err, fd) {
      if (err) return cb(err);

      cb(null, name, fd, _prepareTmpFileRemoveCallback(name, fd, opts));
    });
  });
}

/**
 * Synchronous version of _createTmpFile.
 *
 * @param {Object} options
 * @returns {Object} object consists of name, fd and removeCallback
 * @api private
 */
function _createTmpFileSync(options) {
  var
    args = _parseArguments(options),
    opts = args[0];

    opts.postfix = opts.postfix || '.tmp';

  var name = _getTmpNameSync(opts);
  var fd = fs.openSync(name, CREATE_FLAGS, opts.mode || FILE_MODE);

  return {
    name : name,
    fd : fd,
    removeCallback : _prepareTmpFileRemoveCallback(name, fd, opts)
  };
}

/**
 * Removes files and folders in a directory recursively.
 *
 * @param {String} root
 * @api private
 */
function _rmdirRecursiveSync(root) {
  var dirs = [root];

  do {
    var
      dir = dirs.pop(),
      deferred = false,
      files = fs.readdirSync(dir);

    for (var i = 0, length = files.length; i < length; i++) {
      var
        file = path.join(dir, files[i]),
        stat = fs.lstatSync(file); // lstat so we don't recurse into symlinked directories

      if (stat.isDirectory()) {
        if (!deferred) {
          deferred = true;
          dirs.push(dir);
        }  
        dirs.push(file);
      } else {
        fs.unlinkSync(file);
      }
    }

    if (!deferred) {
      fs.rmdirSync(dir);
    }
  } while (dirs.length !== 0);
}

/**
 * Creates a temporary directory.
 *
 * @param {Object} options
 * @param {Function} callback
 * @api public
 */
function _createTmpDir(options, callback) {
  var
    args = _parseArguments(options, callback),
    opts = args[0],
    cb = args[1];

  // gets a temporary filename
  _getTmpName(opts, function _tmpNameCreated(err, name) {
    if (err) return cb(err);

    // create the directory
    fs.mkdir(name, opts.mode || DIR_MODE, function _dirCreated(err) {
      if (err) return cb(err);

      cb(null, name, _prepareTmpDirRemoveCallback(name, opts));
    });
  });
}

/**
 * Synchronous version of _createTmpDir.
 *
 * @param {Object} options
 * @returns {Object} object consists of name and removeCallback
 * @api private
 */
function _createTmpDirSync(options) {
  var
    args = _parseArguments(options),
    opts = args[0];

  var name = _getTmpNameSync(opts);
  fs.mkdirSync(name, opts.mode || DIR_MODE);

  return {
    name : name,
    removeCallback : _prepareTmpDirRemoveCallback(name, opts)
  };
}

/**
 * Prepares the callback for removal of the temporary file.
 *
 * @param {String} name
 * @param {int} fd
 * @param {Object} opts
 * @api private
 * @returns {Function} the callback
 */
function _prepareTmpFileRemoveCallback(name, fd, opts) {
  var removeCallback = _prepareRemoveCallback(function _removeCallback(fdPath) {
    try {
      fs.closeSync(fdPath[0]);
    }
    catch (e) {
      // under some node/windows related circumstances, a temporary file 
      // may have not be created as expected or the file was already closed
      // by the user, in which case we will simply ignore the error
      if (e.errno != -_c.EBADF && e.errno != -c.ENOENT) {
        // reraise any unanticipated error
        throw e;
      }
    }
    fs.unlinkSync(fdPath[1]);
  }, [fd, name]);

  if (!opts.keep) {
    _removeObjects.unshift(removeCallback);
  }

  return removeCallback;
}

/**
 * Prepares the callback for removal of the temporary directory.
 *
 * @param {String} name
 * @param {Object} opts
 * @returns {Function} the callback
 * @api private
 */
function _prepareTmpDirRemoveCallback(name, opts) {
  var removeFunction = opts.unsafeCleanup ? _rmdirRecursiveSync : fs.rmdirSync.bind(fs);
  var removeCallback = _prepareRemoveCallback(removeFunction, name);

  if (!opts.keep) {
    _removeObjects.unshift(removeCallback);
  }

  return removeCallback;
}

/**
 * Creates a guarded function wrapping the removeFunction call.
 *
 * @param {Function} removeFunction
 * @param {Object} arg
 * @returns {Function}
 * @api private
 */
function _prepareRemoveCallback(removeFunction, arg) {
  var called = false;

  return function _cleanupCallback() {
    if (called) return;

    var index = _removeObjects.indexOf(removeFunction);
    if (index >= 0) {
      _removeObjects.splice(index, 1);
    }

    called = true;
    removeFunction(arg);
  };
}

/**
 * The garbage collector.
 *
 * @api private
 */
function _garbageCollector() {
  if (_uncaughtException && !_gracefulCleanup) {
    return;
  }

  for (var i = 0, length = _removeObjects.length; i < length; i++) {
    try {
      _removeObjects[i].call(null);
    } catch (e) {
      // already removed?
    }
  }
}

function _setGracefulCleanup() {
  _gracefulCleanup = true;
}

var version = process.versions.node.split('.').map(function (value) {
  return parseInt(value, 10);
});

if (version[0] === 0 && (version[1] < 9 || version[1] === 9 && version[2] < 5)) {
  process.addListener('uncaughtException', function _uncaughtExceptionThrown(err) {
    _uncaughtException = true;
    _garbageCollector();

    throw err;
  });
}

process.addListener('exit', function _exit(code) {
  if (code) _uncaughtException = true;
  _garbageCollector();
});

// exporting all the needed methods
module.exports.tmpdir = _TMP;
module.exports.dir = _createTmpDir;
module.exports.dirSync = _createTmpDirSync;
module.exports.file = _createTmpFile;
module.exports.fileSync = _createTmpFileSync;
module.exports.tmpName = _getTmpName;
module.exports.tmpNameSync = _getTmpNameSync;
module.exports.setGracefulCleanup = _setGracefulCleanup;

},{"constants":undefined,"crypto":undefined,"fs":undefined,"os":undefined,"os-tmpdir":17,"path":undefined}],30:[function(require,module,exports){
'use strict';
module.exports = require('os-homedir')();

},{"os-homedir":16}],31:[function(require,module,exports){
/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
'use strict';
var events_1 = require('./events');
var CancellationToken;
(function (CancellationToken) {
    CancellationToken.None = Object.freeze({
        isCancellationRequested: false,
        onCancellationRequested: events_1.Event.None
    });
    CancellationToken.Cancelled = Object.freeze({
        isCancellationRequested: true,
        onCancellationRequested: events_1.Event.None
    });
})(CancellationToken = exports.CancellationToken || (exports.CancellationToken = {}));
var shortcutEvent = Object.freeze(function (callback, context) {
    var handle = setTimeout(callback.bind(context), 0);
    return { dispose: function () { clearTimeout(handle); } };
});
var MutableToken = (function () {
    function MutableToken() {
        this._isCancelled = false;
    }
    MutableToken.prototype.cancel = function () {
        if (!this._isCancelled) {
            this._isCancelled = true;
            if (this._emitter) {
                this._emitter.fire(undefined);
                this._emitter = undefined;
            }
        }
    };
    Object.defineProperty(MutableToken.prototype, "isCancellationRequested", {
        get: function () {
            return this._isCancelled;
        },
        enumerable: true,
        configurable: true
    });
    Object.defineProperty(MutableToken.prototype, "onCancellationRequested", {
        get: function () {
            if (this._isCancelled) {
                return shortcutEvent;
            }
            if (!this._emitter) {
                this._emitter = new events_1.Emitter();
            }
            return this._emitter.event;
        },
        enumerable: true,
        configurable: true
    });
    return MutableToken;
})();
var CancellationTokenSource = (function () {
    function CancellationTokenSource() {
    }
    Object.defineProperty(CancellationTokenSource.prototype, "token", {
        get: function () {
            if (!this._token) {
                // be lazy and create the token only when
                // actually needed
                this._token = new MutableToken();
            }
            return this._token;
        },
        enumerable: true,
        configurable: true
    });
    CancellationTokenSource.prototype.cancel = function () {
        if (!this._token) {
            // save an object by returning the default
            // cancelled token when cancellation happens
            // before someone asks for the token
            this._token = CancellationToken.Cancelled;
        }
        else {
            this._token.cancel();
        }
    };
    CancellationTokenSource.prototype.dispose = function () {
        this.cancel();
    };
    return CancellationTokenSource;
})();
exports.CancellationTokenSource = CancellationTokenSource;

},{"./events":32}],32:[function(require,module,exports){
/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';
var Event;
(function (Event) {
    var _disposable = { dispose: function () { } };
    Event.None = function () { return _disposable; };
})(Event = exports.Event || (exports.Event = {}));
/**
 * Represents a type which can release resources, such
 * as event listening or a timer.
 */
var DisposableImpl = (function () {
    function DisposableImpl(callOnDispose) {
        this._callOnDispose = callOnDispose;
    }
    /**
     * Combine many disposable-likes into one. Use this method
     * when having objects with a dispose function which are not
     * instances of Disposable.
     *
     * @return Returns a new disposable which, upon dispose, will
     * dispose all provides disposable-likes.
     */
    DisposableImpl.from = function () {
        var disposables = [];
        for (var _i = 0; _i < arguments.length; _i++) {
            disposables[_i - 0] = arguments[_i];
        }
        return new DisposableImpl(function () {
            if (disposables) {
                for (var _i = 0; _i < disposables.length; _i++) {
                    var disposable = disposables[_i];
                    disposable.dispose();
                }
                disposables = undefined;
            }
        });
    };
    /**
     * Dispose this object.
     */
    DisposableImpl.prototype.dispose = function () {
        if (typeof this._callOnDispose === 'function') {
            this._callOnDispose();
            this._callOnDispose = undefined;
        }
    };
    return DisposableImpl;
})();
var CallbackList = (function () {
    function CallbackList() {
    }
    CallbackList.prototype.add = function (callback, context, bucket) {
        var _this = this;
        if (context === void 0) { context = null; }
        if (!this._callbacks) {
            this._callbacks = [];
            this._contexts = [];
        }
        this._callbacks.push(callback);
        this._contexts.push(context);
        if (Array.isArray(bucket)) {
            bucket.push({ dispose: function () { return _this.remove(callback, context); } });
        }
    };
    CallbackList.prototype.remove = function (callback, context) {
        if (context === void 0) { context = null; }
        if (!this._callbacks) {
            return;
        }
        var foundCallbackWithDifferentContext = false;
        for (var i = 0, len = this._callbacks.length; i < len; i++) {
            if (this._callbacks[i] === callback) {
                if (this._contexts[i] === context) {
                    // callback & context match => remove it
                    this._callbacks.splice(i, 1);
                    this._contexts.splice(i, 1);
                    return;
                }
                else {
                    foundCallbackWithDifferentContext = true;
                }
            }
        }
        if (foundCallbackWithDifferentContext) {
            throw new Error('When adding a listener with a context, you should remove it with the same context');
        }
    };
    CallbackList.prototype.invoke = function () {
        var args = [];
        for (var _i = 0; _i < arguments.length; _i++) {
            args[_i - 0] = arguments[_i];
        }
        if (!this._callbacks) {
            return;
        }
        var ret = [], callbacks = this._callbacks.slice(0), contexts = this._contexts.slice(0);
        for (var i = 0, len = callbacks.length; i < len; i++) {
            try {
                ret.push(callbacks[i].apply(contexts[i], args));
            }
            catch (e) {
                console.error(e);
            }
        }
        return ret;
    };
    CallbackList.prototype.isEmpty = function () {
        return !this._callbacks || this._callbacks.length === 0;
    };
    CallbackList.prototype.dispose = function () {
        this._callbacks = undefined;
        this._contexts = undefined;
    };
    return CallbackList;
})();
var Emitter = (function () {
    function Emitter(_options) {
        this._options = _options;
    }
    Object.defineProperty(Emitter.prototype, "event", {
        /**
         * For the public to allow to subscribe
         * to events from this Emitter
         */
        get: function () {
            var _this = this;
            if (!this._event) {
                this._event = function (listener, thisArgs, disposables) {
                    if (!_this._callbacks) {
                        _this._callbacks = new CallbackList();
                    }
                    if (_this._options && _this._options.onFirstListenerAdd && _this._callbacks.isEmpty()) {
                        _this._options.onFirstListenerAdd(_this);
                    }
                    _this._callbacks.add(listener, thisArgs);
                    var result;
                    result = {
                        dispose: function () {
                            _this._callbacks.remove(listener, thisArgs);
                            result.dispose = Emitter._noop;
                            if (_this._options && _this._options.onLastListenerRemove && _this._callbacks.isEmpty()) {
                                _this._options.onLastListenerRemove(_this);
                            }
                        }
                    };
                    if (Array.isArray(disposables)) {
                        disposables.push(result);
                    }
                    return result;
                };
            }
            return this._event;
        },
        enumerable: true,
        configurable: true
    });
    /**
     * To be kept private to fire an event to
     * subscribers
     */
    Emitter.prototype.fire = function (event) {
        if (this._callbacks) {
            this._callbacks.invoke.call(this._callbacks, event);
        }
    };
    Emitter.prototype.dispose = function () {
        if (this._callbacks) {
            this._callbacks.dispose();
            this._callbacks = undefined;
        }
    };
    Emitter._noop = function () { };
    return Emitter;
})();
exports.Emitter = Emitter;

},{}],33:[function(require,module,exports){
/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';
var toString = Object.prototype.toString;
function defined(value) {
    return typeof value !== 'undefined';
}
exports.defined = defined;
function undefined(value) {
    return typeof value === 'undefined';
}
exports.undefined = undefined;
function nil(value) {
    return value === null;
}
exports.nil = nil;
function boolean(value) {
    return value === true || value === false;
}
exports.boolean = boolean;
function string(value) {
    return toString.call(value) === '[object String]';
}
exports.string = string;
function number(value) {
    return toString.call(value) === '[object Number]';
}
exports.number = number;
function error(value) {
    return toString.call(value) === '[object Error]';
}
exports.error = error;
function func(value) {
    return toString.call(value) === '[object Function]';
}
exports.func = func;
function array(value) {
    return Array.isArray(value);
}
exports.array = array;
function stringArray(value) {
    return array(value) && value.every(function (elem) { return string(elem); });
}
exports.stringArray = stringArray;

},{}],34:[function(require,module,exports){
/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';
var is = require('./is');
var messages_1 = require('./messages');
exports.ResponseError = messages_1.ResponseError;
exports.ErrorCodes = messages_1.ErrorCodes;
var messageReader_1 = require('./messageReader');
exports.StreamMessageReader = messageReader_1.StreamMessageReader;
exports.IPCMessageReader = messageReader_1.IPCMessageReader;
var messageWriter_1 = require('./messageWriter');
exports.StreamMessageWriter = messageWriter_1.StreamMessageWriter;
exports.IPCMessageWriter = messageWriter_1.IPCMessageWriter;
var events_1 = require('./events');
exports.Event = events_1.Event;
exports.Emitter = events_1.Emitter;
var cancellation_1 = require('./cancellation');
exports.CancellationTokenSource = cancellation_1.CancellationTokenSource;
exports.CancellationToken = cancellation_1.CancellationToken;
var CancelNotification;
(function (CancelNotification) {
    CancelNotification.type = { get method() { return '$/cancelRequest'; } };
})(CancelNotification || (CancelNotification = {}));
(function (Trace) {
    Trace[Trace["Off"] = 0] = "Off";
    Trace[Trace["Messages"] = 1] = "Messages";
    Trace[Trace["Verbose"] = 2] = "Verbose";
})(exports.Trace || (exports.Trace = {}));
var Trace = exports.Trace;
var Trace;
(function (Trace) {
    function fromString(value) {
        value = value.toLowerCase();
        switch (value) {
            case 'off':
                return Trace.Off;
            case 'messages':
                return Trace.Messages;
            case 'verbose':
                return Trace.Verbose;
            default:
                return Trace.Off;
        }
    }
    Trace.fromString = fromString;
})(Trace = exports.Trace || (exports.Trace = {}));
var ConnectionState;
(function (ConnectionState) {
    ConnectionState[ConnectionState["Active"] = 1] = "Active";
    ConnectionState[ConnectionState["Closed"] = 2] = "Closed";
})(ConnectionState || (ConnectionState = {}));
function createMessageConnection(messageReader, messageWriter, logger, client) {
    if (client === void 0) { client = false; }
    var sequenceNumber = 0;
    var version = '2.0';
    var requestHandlers = Object.create(null);
    var eventHandlers = Object.create(null);
    var responsePromises = Object.create(null);
    var requestTokens = Object.create(null);
    var trace = Trace.Off;
    var tracer;
    var state = ConnectionState.Active;
    var errorEmitter = new events_1.Emitter();
    var closeEmitter = new events_1.Emitter();
    function closeHandler() {
        if (state !== ConnectionState.Closed) {
            state = ConnectionState.Closed;
            closeEmitter.fire(undefined);
        }
    }
    ;
    function readErrorHandler(error) {
        errorEmitter.fire([error, undefined, undefined]);
    }
    function writeErrorHandler(data) {
        errorEmitter.fire(data);
    }
    messageReader.onClose(closeHandler);
    messageReader.onError(readErrorHandler);
    messageWriter.onClose(closeHandler);
    messageWriter.onError(writeErrorHandler);
    function handleRequest(requestMessage) {
        function reply(resultOrError) {
            var message = {
                jsonrpc: version,
                id: requestMessage.id
            };
            if (resultOrError instanceof messages_1.ResponseError) {
                message.error = resultOrError.toJson();
            }
            else {
                message.result = is.undefined(resultOrError) ? null : resultOrError;
            }
            messageWriter.write(message);
        }
        function replyError(error) {
            var message = {
                jsonrpc: version,
                id: requestMessage.id,
                error: error.toJson()
            };
            messageWriter.write(message);
        }
        function replySuccess(result) {
            // The JSON RPC defines that a response must either have a result or an error
            // So we can't treat undefined as a valid response result.
            if (is.undefined(result)) {
                result = null;
            }
            var message = {
                jsonrpc: version,
                id: requestMessage.id,
                result: result
            };
            messageWriter.write(message);
        }
        var requestHandler = requestHandlers[requestMessage.method];
        if (requestHandler) {
            var cancellationSource = new cancellation_1.CancellationTokenSource();
            var tokenKey = String(requestMessage.id);
            requestTokens[tokenKey] = cancellationSource;
            try {
                var handlerResult = requestHandler(requestMessage.params, cancellationSource.token);
                var promise = handlerResult;
                if (!handlerResult) {
                    delete requestTokens[tokenKey];
                    replySuccess(handlerResult);
                }
                else if (promise.then) {
                    promise.then(function (resultOrError) {
                        delete requestTokens[tokenKey];
                        reply(resultOrError);
                    }, function (error) {
                        delete requestTokens[tokenKey];
                        if (error instanceof messages_1.ResponseError) {
                            replyError(error);
                        }
                        else if (error && is.string(error.message)) {
                            replyError(new messages_1.ResponseError(messages_1.ErrorCodes.InternalError, "Request " + requestMessage.method + " failed with message: " + error.message));
                        }
                        else {
                            replyError(new messages_1.ResponseError(messages_1.ErrorCodes.InternalError, "Request " + requestMessage.method + " failed unexpectedly without providing any details."));
                        }
                    });
                }
                else {
                    delete requestTokens[tokenKey];
                    reply(handlerResult);
                }
            }
            catch (error) {
                delete requestTokens[tokenKey];
                if (error instanceof messages_1.ResponseError) {
                    reply(error);
                }
                else if (error && is.string(error.message)) {
                    replyError(new messages_1.ResponseError(messages_1.ErrorCodes.InternalError, "Request " + requestMessage.method + " failed with message: " + error.message));
                }
                else {
                    replyError(new messages_1.ResponseError(messages_1.ErrorCodes.InternalError, "Request " + requestMessage.method + " failed unexpectedly without providing any details."));
                }
            }
        }
        else {
            replyError(new messages_1.ResponseError(messages_1.ErrorCodes.MethodNotFound, "Unhandled method " + requestMessage.method));
        }
    }
    function handleResponse(responseMessage) {
        var key = String(responseMessage.id);
        var responsePromise = responsePromises[key];
        if (trace != Trace.Off && tracer) {
            traceResponse(responseMessage, responsePromise);
        }
        if (responsePromise) {
            delete responsePromises[key];
            try {
                if (is.defined(responseMessage.error)) {
                    responsePromise.reject(responseMessage.error);
                }
                else if (is.defined(responseMessage.result)) {
                    responsePromise.resolve(responseMessage.result);
                }
                else {
                    throw new Error('Should never happen.');
                }
            }
            catch (error) {
                if (error.message) {
                    logger.error("Response handler '" + responsePromise.method + "' failed with message: " + error.message);
                }
                else {
                    logger.error("Response handler '" + responsePromise.method + "' failed unexpectedly.");
                }
            }
        }
    }
    function handleNotification(message) {
        var eventHandler;
        if (message.method === CancelNotification.type.method) {
            eventHandler = function (params) {
                var id = params.id;
                var source = requestTokens[String(id)];
                if (source) {
                    source.cancel();
                }
            };
        }
        else {
            eventHandler = eventHandlers[message.method];
        }
        if (eventHandler) {
            try {
                if (trace != Trace.Off && tracer) {
                    traceReceivedNotification(message);
                }
                eventHandler(message.params);
            }
            catch (error) {
                if (error.message) {
                    logger.error("Notification handler '" + message.method + "' failed with message: " + error.message);
                }
                else {
                    logger.error("Notification handler '" + message.method + "' failed unexpectedly.");
                }
            }
        }
    }
    function handleInvalidMessage(message) {
        if (!message) {
            logger.error('Received empty message.');
            return;
        }
        logger.error("Recevied message which is neither a response nor a notification message:\n" + JSON.stringify(message, null, 4));
        // Test whether we find an id to reject the promise
        var responseMessage = message;
        if (is.string(responseMessage.id) || is.number(responseMessage.id)) {
            var key = String(responseMessage.id);
            var responseHandler = responsePromises[key];
            if (responseHandler) {
                responseHandler.reject(new Error('The received response has neither a result nor an error property.'));
            }
        }
    }
    function traceRequest(message) {
        tracer.log("[" + (new Date().toLocaleTimeString()) + "] Sending request '" + message.method + " - (" + message.id + ")'.");
        if (trace === Trace.Verbose && message.params) {
            tracer.log("Params: " + JSON.stringify(message.params, null, 4) + "\n\n");
        }
    }
    function traceSendNotification(message) {
        tracer.log("[" + (new Date().toLocaleTimeString()) + "] Sending notification '" + message.method + "'.");
        if (trace === Trace.Verbose) {
            if (message.params) {
                tracer.log("Params: " + JSON.stringify(message.params, null, 4) + "\n\n");
            }
            else {
                tracer.log('No parameters provided.\n\n');
            }
        }
    }
    function traceReceivedNotification(message) {
        tracer.log("[" + (new Date().toLocaleTimeString()) + "] Received notification '" + message.method + "'.");
        if (trace === Trace.Verbose) {
            if (message.params) {
                tracer.log("Params: " + JSON.stringify(message.params, null, 4) + "\n\n");
            }
            else {
                tracer.log('No parameters provided.\n\n');
            }
        }
    }
    function traceResponse(message, responsePromise) {
        if (responsePromise) {
            var error = message.error ? " Request failed: " + message.error.message + " (" + message.error.code + ")." : '';
            tracer.log("[" + (new Date().toLocaleTimeString()) + "] Recevied response '" + responsePromise.method + " - (" + message.id + ")' in " + (Date.now() - responsePromise.timerStart) + "ms." + error);
        }
        else {
            tracer.log("[" + (new Date().toLocaleTimeString()) + "] Recevied response " + message.id + " without active response promise.");
        }
        if (trace === Trace.Verbose) {
            if (message.error && message.error.data) {
                tracer.log("Error data: " + JSON.stringify(message.error.data, null, 4) + "\n\n");
            }
            else {
                if (message.result) {
                    tracer.log("Result: " + JSON.stringify(message.result, null, 4) + "\n\n");
                }
                else {
                    tracer.log('No result returned.\n\n');
                }
            }
        }
    }
    var callback = function (message) {
        if (messages_1.isRequestMessage(message)) {
            handleRequest(message);
        }
        else if (messages_1.isReponseMessage(message)) {
            handleResponse(message);
        }
        else if (messages_1.isNotificationMessage(message)) {
            handleNotification(message);
        }
        else {
            handleInvalidMessage(message);
        }
    };
    var connection = {
        sendNotification: function (type, params) {
            var notificatioMessage = {
                jsonrpc: version,
                method: type.method,
                params: params
            };
            if (trace != Trace.Off && tracer) {
                traceSendNotification(notificatioMessage);
            }
            messageWriter.write(notificatioMessage);
        },
        onNotification: function (type, handler) {
            eventHandlers[type.method] = handler;
        },
        sendRequest: function (type, params, token) {
            var id = sequenceNumber++;
            var result = new Promise(function (resolve, reject) {
                var requestMessage = {
                    jsonrpc: version,
                    id: id,
                    method: type.method,
                    params: params
                };
                var responsePromise = { method: type.method, timerStart: Date.now(), resolve: resolve, reject: reject };
                if (trace != Trace.Off && tracer) {
                    traceRequest(requestMessage);
                }
                try {
                    messageWriter.write(requestMessage);
                }
                catch (e) {
                    // Writing the message failed. So we need to reject the promise.
                    responsePromise.reject(new messages_1.ResponseError(messages_1.ErrorCodes.MessageWriteError, e.message ? e.message : 'Unknown reason'));
                    responsePromise = null;
                }
                if (responsePromise) {
                    responsePromises[String(id)] = responsePromise;
                }
            });
            if (token) {
                token.onCancellationRequested(function (event) {
                    connection.sendNotification(CancelNotification.type, { id: id });
                });
            }
            return result;
        },
        onRequest: function (type, handler) {
            requestHandlers[type.method] = handler;
        },
        trace: function (_value, _tracer) {
            trace = _value;
            if (trace === Trace.Off) {
                tracer = null;
            }
            else {
                tracer = _tracer;
            }
        },
        onError: errorEmitter.event,
        onClose: closeEmitter.event,
        dispose: function () {
        },
        listen: function () {
            messageReader.listen(callback);
        }
    };
    return connection;
}
function isMessageReader(value) {
    return is.defined(value.listen) && is.undefined(value.read);
}
function isMessageWriter(value) {
    return is.defined(value.write) && is.undefined(value.end);
}
function createServerMessageConnection(input, output, logger) {
    var reader = isMessageReader(input) ? input : new messageReader_1.StreamMessageReader(input);
    var writer = isMessageWriter(output) ? output : new messageWriter_1.StreamMessageWriter(output);
    return createMessageConnection(reader, writer, logger);
}
exports.createServerMessageConnection = createServerMessageConnection;
function createClientMessageConnection(input, output, logger) {
    var reader = isMessageReader(input) ? input : new messageReader_1.StreamMessageReader(input);
    var writer = isMessageWriter(output) ? output : new messageWriter_1.StreamMessageWriter(output);
    return createMessageConnection(reader, writer, logger, true);
}
exports.createClientMessageConnection = createClientMessageConnection;

},{"./cancellation":31,"./events":32,"./is":33,"./messageReader":35,"./messageWriter":36,"./messages":37}],35:[function(require,module,exports){
/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';
var __extends = (this && this.__extends) || function (d, b) {
    for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
    function __() { this.constructor = d; }
    d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
};
var events_1 = require('./events');
var is = require('./is');
var DefaultSize = 8192;
var CR = new Buffer('\r', 'ascii')[0];
var LF = new Buffer('\n', 'ascii')[0];
var CRLF = '\r\n';
var MessageBuffer = (function () {
    function MessageBuffer(encoding) {
        if (encoding === void 0) { encoding = 'utf-8'; }
        this.encoding = encoding;
        this.index = 0;
        this.buffer = new Buffer(DefaultSize);
    }
    MessageBuffer.prototype.append = function (chunk) {
        var toAppend = chunk;
        if (typeof (chunk) == 'string') {
            var str = chunk;
            toAppend = new Buffer(str.length);
            toAppend.write(str, 0, str.length, this.encoding);
        }
        if (this.buffer.length - this.index >= toAppend.length) {
            toAppend.copy(this.buffer, this.index, 0, toAppend.length);
        }
        else {
            var newSize = (Math.ceil((this.index + toAppend.length) / DefaultSize) + 1) * DefaultSize;
            if (this.index === 0) {
                this.buffer = new Buffer(newSize);
                toAppend.copy(this.buffer, 0, 0, toAppend.length);
            }
            else {
                this.buffer = Buffer.concat([this.buffer.slice(0, this.index), toAppend], newSize);
            }
        }
        this.index += toAppend.length;
    };
    MessageBuffer.prototype.tryReadHeaders = function () {
        var result = undefined;
        var current = 0;
        while (current + 3 < this.index && (this.buffer[current] !== CR || this.buffer[current + 1] !== LF || this.buffer[current + 2] !== CR || this.buffer[current + 3] !== LF)) {
            current++;
        }
        // No header / body separator found (e.g CRLFCRLF)
        if (current + 3 >= this.index) {
            return result;
        }
        result = Object.create(null);
        var headers = this.buffer.toString('ascii', 0, current).split(CRLF);
        headers.forEach(function (header) {
            var index = header.indexOf(':');
            if (index === -1) {
                throw new Error('Message header must separate key and value using :');
            }
            var key = header.substr(0, index);
            var value = header.substr(index + 1).trim();
            result[key] = value;
        });
        var nextStart = current + 4;
        this.buffer = this.buffer.slice(nextStart);
        this.index = this.index - nextStart;
        return result;
    };
    MessageBuffer.prototype.tryReadContent = function (length) {
        if (this.index < length) {
            return null;
        }
        var result = this.buffer.toString(this.encoding, 0, length);
        var nextStart = length;
        this.buffer.copy(this.buffer, 0, nextStart);
        this.index = this.index - nextStart;
        return result;
    };
    Object.defineProperty(MessageBuffer.prototype, "numberOfBytes", {
        get: function () {
            return this.index;
        },
        enumerable: true,
        configurable: true
    });
    return MessageBuffer;
})();
var AbstractMessageReader = (function () {
    function AbstractMessageReader() {
        this.errorEmitter = new events_1.Emitter();
        this.closeEmitter = new events_1.Emitter();
    }
    Object.defineProperty(AbstractMessageReader.prototype, "onError", {
        get: function () {
            return this.errorEmitter.event;
        },
        enumerable: true,
        configurable: true
    });
    AbstractMessageReader.prototype.fireError = function (error) {
        this.errorEmitter.fire(this.asError(error));
    };
    Object.defineProperty(AbstractMessageReader.prototype, "onClose", {
        get: function () {
            return this.closeEmitter.event;
        },
        enumerable: true,
        configurable: true
    });
    AbstractMessageReader.prototype.fireClose = function () {
        this.closeEmitter.fire(undefined);
    };
    AbstractMessageReader.prototype.asError = function (error) {
        if (error instanceof Error) {
            return error;
        }
        else {
            return new Error("Reader recevied error. Reason: " + (is.string(error.message) ? error.message : 'unknown'));
        }
    };
    return AbstractMessageReader;
})();
exports.AbstractMessageReader = AbstractMessageReader;
var StreamMessageReader = (function (_super) {
    __extends(StreamMessageReader, _super);
    function StreamMessageReader(readable, encoding) {
        if (encoding === void 0) { encoding = 'utf-8'; }
        _super.call(this);
        this.readable = readable;
        this.buffer = new MessageBuffer(encoding);
    }
    StreamMessageReader.prototype.listen = function (callback) {
        var _this = this;
        this.nextMessageLength = -1;
        this.callback = callback;
        this.readable.on('data', function (data) {
            _this.onData(data);
        });
        this.readable.on('error', function (error) { return _this.fireError(error); });
        this.readable.on('close', function () { return _this.fireClose(); });
    };
    StreamMessageReader.prototype.onData = function (data) {
        this.buffer.append(data);
        while (true) {
            if (this.nextMessageLength === -1) {
                var headers = this.buffer.tryReadHeaders();
                if (!headers) {
                    return;
                }
                var contentLength = headers['Content-Length'];
                if (!contentLength) {
                    throw new Error('Header must provide a Content-Length property.');
                }
                var length_1 = parseInt(contentLength);
                if (isNaN(length_1)) {
                    throw new Error('Content-Length value must be a number.');
                }
                this.nextMessageLength = length_1;
            }
            var msg = this.buffer.tryReadContent(this.nextMessageLength);
            if (msg === null) {
                return;
            }
            this.nextMessageLength = -1;
            var json = JSON.parse(msg);
            this.callback(json);
        }
    };
    return StreamMessageReader;
})(AbstractMessageReader);
exports.StreamMessageReader = StreamMessageReader;
var IPCMessageReader = (function (_super) {
    __extends(IPCMessageReader, _super);
    function IPCMessageReader(process) {
        var _this = this;
        _super.call(this);
        this.process = process;
        this.process.on('error', function (error) { return _this.fireError(error); });
        this.process.on('close', function () { return _this.fireClose(); });
    }
    IPCMessageReader.prototype.listen = function (callback) {
        this.process.on('message', callback);
    };
    return IPCMessageReader;
})(AbstractMessageReader);
exports.IPCMessageReader = IPCMessageReader;

},{"./events":32,"./is":33}],36:[function(require,module,exports){
/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';
var __extends = (this && this.__extends) || function (d, b) {
    for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
    function __() { this.constructor = d; }
    d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
};
var events_1 = require('./events');
var is = require('./is');
var ContentLength = 'Content-Length: ';
var CRLF = '\r\n';
var AbstractMessageWriter = (function () {
    function AbstractMessageWriter() {
        this.errorEmitter = new events_1.Emitter();
        this.closeEmitter = new events_1.Emitter();
    }
    Object.defineProperty(AbstractMessageWriter.prototype, "onError", {
        get: function () {
            return this.errorEmitter.event;
        },
        enumerable: true,
        configurable: true
    });
    AbstractMessageWriter.prototype.fireError = function (error, message, count) {
        this.errorEmitter.fire([this.asError(error), message, count]);
    };
    Object.defineProperty(AbstractMessageWriter.prototype, "onClose", {
        get: function () {
            return this.closeEmitter.event;
        },
        enumerable: true,
        configurable: true
    });
    AbstractMessageWriter.prototype.fireClose = function () {
        this.closeEmitter.fire(undefined);
    };
    AbstractMessageWriter.prototype.asError = function (error) {
        if (error instanceof Error) {
            return error;
        }
        else {
            return new Error("Writer recevied error. Reason: " + (is.string(error.message) ? error.message : 'unknown'));
        }
    };
    return AbstractMessageWriter;
})();
exports.AbstractMessageWriter = AbstractMessageWriter;
var StreamMessageWriter = (function (_super) {
    __extends(StreamMessageWriter, _super);
    function StreamMessageWriter(writable, encoding) {
        var _this = this;
        if (encoding === void 0) { encoding = 'utf8'; }
        _super.call(this);
        this.writable = writable;
        this.encoding = encoding;
        this.errorCount = 0;
        this.writable.on('error', function (error) { return _this.fireError(error); });
        this.writable.on('close', function () { return _this.fireClose(); });
    }
    StreamMessageWriter.prototype.write = function (msg) {
        var json = JSON.stringify(msg);
        var contentLength = Buffer.byteLength(json, this.encoding);
        var headers = [
            ContentLength, contentLength.toString(), CRLF,
            CRLF
        ];
        try {
            // Header must be written in ASCII encoding
            this.writable.write(headers.join(''), 'ascii');
            // Now write the content. This can be written in any encoding
            this.writable.write(json, this.encoding);
            this.errorCount = 0;
        }
        catch (error) {
            this.errorCount++;
            this.fireError(error, msg, this.errorCount);
        }
    };
    return StreamMessageWriter;
})(AbstractMessageWriter);
exports.StreamMessageWriter = StreamMessageWriter;
var IPCMessageWriter = (function (_super) {
    __extends(IPCMessageWriter, _super);
    function IPCMessageWriter(process) {
        var _this = this;
        _super.call(this);
        this.process = process;
        this.errorCount = 0;
        this.process.on('error', function (error) { return _this.fireError(error); });
        this.process.on('close', function () { return _this.fireClose; });
    }
    IPCMessageWriter.prototype.write = function (msg) {
        try {
            this.process.send(msg);
            this.errorCount = 0;
        }
        catch (error) {
            this.errorCount++;
            this.fireError(error, msg, this.errorCount);
        }
    };
    return IPCMessageWriter;
})(AbstractMessageWriter);
exports.IPCMessageWriter = IPCMessageWriter;

},{"./events":32,"./is":33}],37:[function(require,module,exports){
/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';
var __extends = (this && this.__extends) || function (d, b) {
    for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p];
    function __() { this.constructor = d; }
    d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
};
var is = require('./is');
/**
 * Predefined error codes.
 */
var ErrorCodes;
(function (ErrorCodes) {
    // Defined by JSON RPC
    ErrorCodes.ParseError = -32700;
    ErrorCodes.InvalidRequest = -32600;
    ErrorCodes.MethodNotFound = -32601;
    ErrorCodes.InvalidParams = -32602;
    ErrorCodes.InternalError = -32603;
    ErrorCodes.serverErrorStart = -32099;
    ErrorCodes.serverErrorEnd = -32000;
    // Defined by VSCode.
    ErrorCodes.MessageWriteError = 1;
    ErrorCodes.MessageReadError = 2;
})(ErrorCodes = exports.ErrorCodes || (exports.ErrorCodes = {}));
/**
 * A error object return in a response in case a request
 * has failed.
 */
var ResponseError = (function (_super) {
    __extends(ResponseError, _super);
    function ResponseError(code, message, data) {
        _super.call(this, message);
        this.code = code;
        this.message = message;
        if (is.defined(data)) {
            this.data = data;
        }
    }
    ResponseError.prototype.toJson = function () {
        var result = {
            code: this.code,
            message: this.message
        };
        if (is.defined(this.data)) {
            result.data = this.data;
        }
        ;
        return result;
    };
    return ResponseError;
})(Error);
exports.ResponseError = ResponseError;
/**
 * Tests if the given message is a request message
 */
function isRequestMessage(message) {
    var candidate = message;
    return candidate && is.string(candidate.method) && (is.string(candidate.id) || is.number(candidate.id));
}
exports.isRequestMessage = isRequestMessage;
/**
 * Tests if the given message is a notification message
 */
function isNotificationMessage(message) {
    var candidate = message;
    return candidate && is.string(candidate.method) && is.undefined(message.id);
}
exports.isNotificationMessage = isNotificationMessage;
/**
 * Tests if the given message is a response message
 */
function isReponseMessage(message) {
    var candidate = message;
    return candidate && (is.defined(candidate.result) || is.defined(candidate.error)) && (is.string(candidate.id) || is.number(candidate.id));
}
exports.isReponseMessage = isReponseMessage;

},{"./is":33}],38:[function(require,module,exports){
/* --------------------------------------------------------------------------------------------
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License. See License.txt in the project root for license information.
 * ------------------------------------------------------------------------------------------ */
'use strict';
var path = require('path');
var fs = require('fs');
var _options = { locale: undefined, cacheLanguageResolution: true };
var _isPseudo = false;
var _resolvedLanguage = null;
var toString = Object.prototype.toString;
function isDefined(value) {
    return typeof value !== 'undefined';
}
function isNumber(value) {
    return toString.call(value) === '[object Number]';
}
function isString(value) {
    return toString.call(value) === '[object String]';
}
function isBoolean(value) {
    return value === true || value === false;
}
function format(message, args) {
    var result;
    if (_isPseudo) {
        // FF3B and FF3D is the Unicode zenkaku representation for [ and ]
        message = '\uFF3B' + message.replace(/[aouei]/g, '$&$&') + '\uFF3D';
    }
    if (args.length === 0) {
        result = message;
    }
    else {
        result = message.replace(/\{(\d+)\}/g, function (match, rest) {
            var index = rest[0];
            return typeof args[index] !== 'undefined' ? args[index] : match;
        });
    }
    return result;
}
function createScopedLocalizeFunction(messages) {
    return function (key, message) {
        var args = [];
        for (var _i = 2; _i < arguments.length; _i++) {
            args[_i - 2] = arguments[_i];
        }
        if (isNumber(key)) {
            if (key >= messages.length) {
                console.error("Broken localize call found. Index out of bounds. Stacktrace is\n: " + (new Error('')).stack);
                return;
            }
            return format(messages[key], args);
        }
        else {
            if (isString(message)) {
                console.warn("Message " + message + " didn't get externalized correctly.");
                return format(message, args);
            }
            else {
                console.error("Broken localize call found. Stacktrace is\n: " + (new Error('')).stack);
            }
        }
    };
}
function localize(key, message) {
    var args = [];
    for (var _i = 2; _i < arguments.length; _i++) {
        args[_i - 2] = arguments[_i];
    }
    return format(message, args);
}
function resolveLanguage(file) {
    var ext = path.extname(file);
    if (ext) {
        file = file.substr(0, file.length - ext.length);
    }
    var resolvedLanguage;
    if (_options.cacheLanguageResolution && _resolvedLanguage) {
        resolvedLanguage = _resolvedLanguage;
    }
    else {
        if (_isPseudo || !_options.locale) {
            resolvedLanguage = '.nls.json';
        }
        else {
            var locale = _options.locale;
            while (locale) {
                var candidate = '.nls.' + locale + '.json';
                if (fs.existsSync(file + candidate)) {
                    resolvedLanguage = candidate;
                    break;
                }
                else {
                    var index = locale.lastIndexOf('-');
                    if (index > 0) {
                        locale = locale.substring(0, index);
                    }
                    else {
                        resolvedLanguage = '.nls.json';
                        locale = null;
                    }
                }
            }
        }
        if (_options.cacheLanguageResolution) {
            _resolvedLanguage = resolvedLanguage;
        }
    }
    return file + resolvedLanguage;
}
function loadMessageBundle(file) {
    if (!file) {
        return localize;
    }
    else {
        var resolvedFile = resolveLanguage(file);
        try {
            var json = require(resolvedFile);
            if (Array.isArray(json)) {
                return createScopedLocalizeFunction(json);
            }
            else {
                if (isDefined(json.messages) && isDefined(json.keys)) {
                    return createScopedLocalizeFunction(json.messages);
                }
                else {
                    console.error("String bundle '" + file + "' uses an unsupported format.");
                    return localize;
                }
            }
        }
        catch (e) {
            console.error("Can't load string bundle for " + file);
            return localize;
        }
    }
}
exports.loadMessageBundle = loadMessageBundle;
function config(opt) {
    var options;
    if (isString(opt)) {
        try {
            options = JSON.parse(opt);
        }
        catch (e) {
            console.error("Error parsing nls options: " + opt);
        }
    }
    else {
        options = opt;
    }
    if (options) {
        if (isString(options.locale)) {
            _options.locale = options.locale.toLowerCase();
            _resolvedLanguage = null;
        }
        if (isBoolean(options.cacheLanguageResolution)) {
            _options.cacheLanguageResolution = options.cacheLanguageResolution;
        }
    }
    _isPseudo = _options.locale === 'pseudo';
    return loadMessageBundle;
}
exports.config = config;

},{"fs":undefined,"path":undefined}]},{},[5])
//# sourceMappingURL=hubHost.all.js.map

// SIG // Begin signature block
// SIG // MIIhdAYJKoZIhvcNAQcCoIIhZTCCIWECAQExDzANBglg
// SIG // hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
// SIG // BgEEAYI3AgEeMCQCAQEEEBDgyQbOONQRoqMAEEvTUJAC
// SIG // AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
// SIG // fOgEJ+ZqmnAcOj2mC+LZGFVmRaKScAuTM+oLYEcst7qg
// SIG // ggtyMIIE+jCCA+KgAwIBAgITMwAAA96NVoJa8aSpZwAA
// SIG // AAAD3jANBgkqhkiG9w0BAQsFADB+MQswCQYDVQQGEwJV
// SIG // UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
// SIG // UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
// SIG // cmF0aW9uMSgwJgYDVQQDEx9NaWNyb3NvZnQgQ29kZSBT
// SIG // aWduaW5nIFBDQSAyMDEwMB4XDTIwMTIxNTIxMjQyMFoX
// SIG // DTIxMTIwMjIxMjQyMFowdDELMAkGA1UEBhMCVVMxEzAR
// SIG // BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
// SIG // bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
// SIG // bjEeMBwGA1UEAxMVTWljcm9zb2Z0IENvcnBvcmF0aW9u
// SIG // MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA
// SIG // p/Pw4knSJEizXv2opKMZFiALQqvVvu/nWxFzf3GeGZHl
// SIG // /5p/DD0SVcrexWhN5GxaHw5eBh+cWTkKDlIRofJn6t7F
// SIG // KvUQ2ek9bxWp1U3kat4mpr5o2B1YGKtTFvUXWNVGhh+z
// SIG // hNi9HKBo6eOms1Z2CZY+yNQVjGV8WEUsRkLbTYvYNZcF
// SIG // whnxpfmk6lkn8oUaTAf5qVQuCBfDGLB1WHLWG9Xsft2c
// SIG // A0psLF/2OLMbUqbdfMV/UbKNnxEvBhp0CnQZzDHckoHa
// SIG // dBOAAlqWtIkA0wUDEFUk361RY8nrb4h6DHoyPkoG5Ij6
// SIG // xHsxhr04hjqHOvnlkhMGDMvbMcNBmgolbwIDAQABo4IB
// SIG // eTCCAXUwHwYDVR0lBBgwFgYKKwYBBAGCNz0GAQYIKwYB
// SIG // BQUHAwMwHQYDVR0OBBYEFCTqO9HxWq7s5cRu7vkdh3fT
// SIG // dUyJMFAGA1UdEQRJMEekRTBDMSkwJwYDVQQLEyBNaWNy
// SIG // b3NvZnQgT3BlcmF0aW9ucyBQdWVydG8gUmljbzEWMBQG
// SIG // A1UEBRMNMjMwODY1KzQ2MzEzMzAfBgNVHSMEGDAWgBTm
// SIG // /F97uyIAWORyTrX0IXQjMubvrDBWBgNVHR8ETzBNMEug
// SIG // SaBHhkVodHRwOi8vY3JsLm1pY3Jvc29mdC5jb20vcGtp
// SIG // L2NybC9wcm9kdWN0cy9NaWNDb2RTaWdQQ0FfMjAxMC0w
// SIG // Ny0wNi5jcmwwWgYIKwYBBQUHAQEETjBMMEoGCCsGAQUF
// SIG // BzAChj5odHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtp
// SIG // L2NlcnRzL01pY0NvZFNpZ1BDQV8yMDEwLTA3LTA2LmNy
// SIG // dDAMBgNVHRMBAf8EAjAAMA0GCSqGSIb3DQEBCwUAA4IB
// SIG // AQA8X/AOtE0n612ubeRtRMSdPDqYwovUQbDfE/sJKM3M
// SIG // 9RNEHwfJsWhN/LtNEvPc/h6AiefywoDAWFaE1gB7WxuD
// SIG // 2E+6y3Bswbl/EvjiJQUdi7YIWeR+pgom5aQ/93TM9ndm
// SIG // etv0wmAB6dIipR2vhCfylu0j7L31DcUNiG6Jb130Brk8
// SIG // hVfRxDfzXX5lloO/L11WduU5TNRy47qLRoMhu03d2+HB
// SIG // 0AWWzvl1WfwCE7HwAek5ahcxgiyDWMmfdBF9LAHJuplb
// SIG // zthg1D4qiYyqIftJztOnHBEwnsjDEcbeqMiQftFJJXaS
// SIG // l+GmXNfLtIDaIRxsdOG6fSXOg07k4d8pxPIcMIIGcDCC
// SIG // BFigAwIBAgIKYQxSTAAAAAAAAzANBgkqhkiG9w0BAQsF
// SIG // ADCBiDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hp
// SIG // bmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoT
// SIG // FU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEyMDAGA1UEAxMp
// SIG // TWljcm9zb2Z0IFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9y
// SIG // aXR5IDIwMTAwHhcNMTAwNzA2MjA0MDE3WhcNMjUwNzA2
// SIG // MjA1MDE3WjB+MQswCQYDVQQGEwJVUzETMBEGA1UECBMK
// SIG // V2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwG
// SIG // A1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSgwJgYD
// SIG // VQQDEx9NaWNyb3NvZnQgQ29kZSBTaWduaW5nIFBDQSAy
// SIG // MDEwMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC
// SIG // AQEA6Q5kUHlntcTj/QkATJ6UrPdWaOpE2M/FWE+ppXZ8
// SIG // bUW60zmStKQe+fllguQX0o/9RJwI6GWTzixVhL99COMu
// SIG // K6hBKxi3oktuSUxrFQfe0dLCiR5xlM21f0u0rwjYzIjW
// SIG // axeUOpPOJj/s5v40mFfVHV1J9rIqLtWFu1k/+JC0K4N0
// SIG // yiuzO0bj8EZJwRdmVMkcvR3EVWJXcvhnuSUgNN5dpqWV
// SIG // XqsogM3Vsp7lA7Vj07IUyMHIiiYKWX8H7P8O7YASNUwS
// SIG // pr5SW/Wm2uCLC0h31oVH1RC5xuiq7otqLQVcYMa0Kluc
// SIG // IxxfReMaFB5vN8sZM4BqiU2jamZjeJPVMM+VHwIDAQAB
// SIG // o4IB4zCCAd8wEAYJKwYBBAGCNxUBBAMCAQAwHQYDVR0O
// SIG // BBYEFOb8X3u7IgBY5HJOtfQhdCMy5u+sMBkGCSsGAQQB
// SIG // gjcUAgQMHgoAUwB1AGIAQwBBMAsGA1UdDwQEAwIBhjAP
// SIG // BgNVHRMBAf8EBTADAQH/MB8GA1UdIwQYMBaAFNX2VsuP
// SIG // 6KJcYmjRPZSQW9fOmhjEMFYGA1UdHwRPME0wS6BJoEeG
// SIG // RWh0dHA6Ly9jcmwubWljcm9zb2Z0LmNvbS9wa2kvY3Js
// SIG // L3Byb2R1Y3RzL01pY1Jvb0NlckF1dF8yMDEwLTA2LTIz
// SIG // LmNybDBaBggrBgEFBQcBAQROMEwwSgYIKwYBBQUHMAKG
// SIG // Pmh0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2kvY2Vy
// SIG // dHMvTWljUm9vQ2VyQXV0XzIwMTAtMDYtMjMuY3J0MIGd
// SIG // BgNVHSAEgZUwgZIwgY8GCSsGAQQBgjcuAzCBgTA9Bggr
// SIG // BgEFBQcCARYxaHR0cDovL3d3dy5taWNyb3NvZnQuY29t
// SIG // L1BLSS9kb2NzL0NQUy9kZWZhdWx0Lmh0bTBABggrBgEF
// SIG // BQcCAjA0HjIgHQBMAGUAZwBhAGwAXwBQAG8AbABpAGMA
// SIG // eQBfAFMAdABhAHQAZQBtAGUAbgB0AC4gHTANBgkqhkiG
// SIG // 9w0BAQsFAAOCAgEAGnTvV08pe8QWhXi4UNMi/AmdrIKX
// SIG // +DT/KiyXlRLl5L/Pv5PI4zSp24G43B4AvtI1b6/lf3mV
// SIG // d+UC1PHr2M1OHhthosJaIxrwjKhiUUVnCOM/PB6T+DCF
// SIG // F8g5QKbXDrMhKeWloWmMIpPMdJjnoUdD8lOswA8waX/+
// SIG // 0iUgbW9h098H1dlyACxphnY9UdumOUjJN2FtB91TGcun
// SIG // 1mHCv+KDqw/ga5uV1n0oUbCJSlGkmmzItx9KGg5pqdfc
// SIG // wX7RSXCqtq27ckdjF/qm1qKmhuyoEESbY7ayaYkGx0aG
// SIG // ehg/6MUdIdV7+QIjLcVBy78dTMgW77Gcf/wiS0mKbhXj
// SIG // pn92W9FTeZGFndXS2z1zNfM8rlSyUkdqwKoTldKOEdqZ
// SIG // Z14yjPs3hdHcdYWch8ZaV4XCv90Nj4ybLeu07s8n07Ve
// SIG // afqkFgQBpyRnc89NT7beBVaXevfpUk30dwVPhcbYC/GO
// SIG // 7UIJ0Q124yNWeCImNr7KsYxuqh3khdpHM2KPpMmRM19x
// SIG // HkCvmGXJIuhCISWKHC1g2TeJQYkqFg/XYTyUaGBS79ZH
// SIG // maCAQO4VgXc+nOBTGBpQHTiVmx5mMxMnORd4hzbOTsNf
// SIG // svU9R1O24OXbC2E9KteSLM43Wj5AQjGkHxAIwlacvyRd
// SIG // UQKdannSF9PawZSOB3slcUSrBmrm1MbfI5qWdcUxghVa
// SIG // MIIVVgIBATCBlTB+MQswCQYDVQQGEwJVUzETMBEGA1UE
// SIG // CBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEe
// SIG // MBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSgw
// SIG // JgYDVQQDEx9NaWNyb3NvZnQgQ29kZSBTaWduaW5nIFBD
// SIG // QSAyMDEwAhMzAAAD3o1WglrxpKlnAAAAAAPeMA0GCWCG
// SIG // SAFlAwQCAQUAoIGuMBkGCSqGSIb3DQEJAzEMBgorBgEE
// SIG // AYI3AgEEMBwGCisGAQQBgjcCAQsxDjAMBgorBgEEAYI3
// SIG // AgEVMC8GCSqGSIb3DQEJBDEiBCCOfsfjTy/SgHDwnQA2
// SIG // uuMz2lxlycbH8d52grJW9li13DBCBgorBgEEAYI3AgEM
// SIG // MTQwMqAUgBIATQBpAGMAcgBvAHMAbwBmAHShGoAYaHR0
// SIG // cDovL3d3dy5taWNyb3NvZnQuY29tMA0GCSqGSIb3DQEB
// SIG // AQUABIIBADT5mgN/3JvKZljnlfINU+FsGHxltv6NSeCS
// SIG // OM5Se+/Ev4Z5PEFxae+auRfbVyWfOmQD2a2mHwa6/0iD
// SIG // xtysAxgxxvYMyep/+wBgy2pu/zFcocKBqaye5A29c79N
// SIG // nQBtPnY6CfmWy/iRzwD1KiMS74q4e2JHHmS2mOhdC7+e
// SIG // r2bbqt5Z6ecwzeiZP/hkikkfg1UcHFA0+icM6LbWCdEL
// SIG // S/AUQTjgTpcdrUImHcSMm87fAf6Xv/tGdGUd+1LZP4RB
// SIG // Yy+oIEiC9eV3dtnjjZNH5A4Co/O6TYQiM5UhArJEZ8D/
// SIG // WTxRmH/TKsuIwvvd7P25DUuKeQYgDt9TWxMkzLx2XB2h
// SIG // ghLkMIIS4AYKKwYBBAGCNwMDATGCEtAwghLMBgkqhkiG
// SIG // 9w0BBwKgghK9MIISuQIBAzEPMA0GCWCGSAFlAwQCAQUA
// SIG // MIIBUAYLKoZIhvcNAQkQAQSgggE/BIIBOzCCATcCAQEG
// SIG // CisGAQQBhFkKAwEwMTANBglghkgBZQMEAgEFAAQgCDVW
// SIG // WLQ/xtcEgxZCOTXi3L/EfYtHEMhx2GSQsv9nGgcCBmCJ
// SIG // 1kQiLxgSMjAyMTA1MTMwMDA0NTMuMTFaMASAAgH0oIHQ
// SIG // pIHNMIHKMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
// SIG // aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
// SIG // ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSUwIwYDVQQL
// SIG // ExxNaWNyb3NvZnQgQW1lcmljYSBPcGVyYXRpb25zMSYw
// SIG // JAYDVQQLEx1UaGFsZXMgVFNTIEVTTjo3QkYxLUUzRUEt
// SIG // QjgwODElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUtU3Rh
// SIG // bXAgU2VydmljZaCCDjwwggTxMIID2aADAgECAhMzAAAB
// SIG // UcNQ51lsqsanAAAAAAFRMA0GCSqGSIb3DQEBCwUAMHwx
// SIG // CzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9u
// SIG // MRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNy
// SIG // b3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1pY3Jv
// SIG // c29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMB4XDTIwMTEx
// SIG // MjE4MjYwNFoXDTIyMDIxMTE4MjYwNFowgcoxCzAJBgNV
// SIG // BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYD
// SIG // VQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQg
// SIG // Q29ycG9yYXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBB
// SIG // bWVyaWNhIE9wZXJhdGlvbnMxJjAkBgNVBAsTHVRoYWxl
// SIG // cyBUU1MgRVNOOjdCRjEtRTNFQS1CODA4MSUwIwYDVQQD
// SIG // ExxNaWNyb3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNlMIIB
// SIG // IjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAn9KH
// SIG // 76qErjvvOIkjWbHptMkYDjmG+JEmzguyr/VxjZgZ/ig8
// SIG // Mk47jqSJP5RxH/sDyqhYu7jPSO86siZh8u7DBX9L8I+A
// SIG // B+8fPPvD4uoLKD22BpoFl4B8Fw5K7SuibvbxGN7adL1/
// SIG // zW+sWXlVvpDhEPIKDICvEdNjGTLhktfftjefg9lumBMU
// SIG // BJ2G4/g4ad0dDvRNmKiMZXXe/Ll4Qg/oPSzXCUEYoSSq
// SIG // a5D+5MRimVe5/YTLj0jVr8iF45V0hT7VH8OJO4YImcnZ
// SIG // hq6Dw1G+w6ACRGePFmOWqW8tEZ13SMmOquJrTkwyy8zy
// SIG // NtVttJAX7diFLbR0SvMlbJZWK0KHdwIDAQABo4IBGzCC
// SIG // ARcwHQYDVR0OBBYEFMV3/+NoUGKTNGg6OMyE6fN1ROpt
// SIG // MB8GA1UdIwQYMBaAFNVjOlyKMZDzQ3t8RhvFM2hahW1V
// SIG // MFYGA1UdHwRPME0wS6BJoEeGRWh0dHA6Ly9jcmwubWlj
// SIG // cm9zb2Z0LmNvbS9wa2kvY3JsL3Byb2R1Y3RzL01pY1Rp
// SIG // bVN0YVBDQV8yMDEwLTA3LTAxLmNybDBaBggrBgEFBQcB
// SIG // AQROMEwwSgYIKwYBBQUHMAKGPmh0dHA6Ly93d3cubWlj
// SIG // cm9zb2Z0LmNvbS9wa2kvY2VydHMvTWljVGltU3RhUENB
// SIG // XzIwMTAtMDctMDEuY3J0MAwGA1UdEwEB/wQCMAAwEwYD
// SIG // VR0lBAwwCgYIKwYBBQUHAwgwDQYJKoZIhvcNAQELBQAD
// SIG // ggEBACv99cAVg5nx0SqjvLfQzmugMj5cJ9NE60duSH1L
// SIG // pxHYim9Ls3UfiYd7t0JvyEw/rRTEKHbznV6LFLlX++lH
// SIG // JMGKzZnHtTe2OI6ZHFnNiFhtgyWuYDJrm7KQykNi1G1L
// SIG // buVie9MehmoK+hBiZnnrcfZSnBSokrvO2QEWHC1xnZ5w
// SIG // M82UEjprFYOkchU+6RcoCjjmIFGfgSzNj1MIbf4lcJ5F
// SIG // oV1Mg6FwF45CijOXHVXrzkisMZ9puDpFjjEV6TAY6INg
// SIG // MkhLev/AVow0sF8MfQztJIlFYdFEkZ5NF/IyzoC2Yb9i
// SIG // w4bCKdBrdD3As6mvoGSNjCC6lOdz6EerJK3NhFgwggZx
// SIG // MIIEWaADAgECAgphCYEqAAAAAAACMA0GCSqGSIb3DQEB
// SIG // CwUAMIGIMQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
// SIG // aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
// SIG // ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMTIwMAYDVQQD
// SIG // EylNaWNyb3NvZnQgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRo
// SIG // b3JpdHkgMjAxMDAeFw0xMDA3MDEyMTM2NTVaFw0yNTA3
// SIG // MDEyMTQ2NTVaMHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
// SIG // EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
// SIG // HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAk
// SIG // BgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAy
// SIG // MDEwMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC
// SIG // AQEAqR0NvHcRijog7PwTl/X6f2mUa3RUENWlCgCChfvt
// SIG // fGhLLF/Fw+Vhwna3PmYrW/AVUycEMR9BGxqVHc4JE458
// SIG // YTBZsTBED/FgiIRUQwzXTbg4CLNC3ZOs1nMwVyaCo0UN
// SIG // 0Or1R4HNvyRgMlhgRvJYR4YyhB50YWeRX4FUsc+TTJLB
// SIG // xKZd0WETbijGGvmGgLvfYfxGwScdJGcSchohiq9LZIlQ
// SIG // YrFd/XcfPfBXday9ikJNQFHRD5wGPmd/9WbAA5ZEfu/Q
// SIG // S/1u5ZrKsajyeioKMfDaTgaRtogINeh4HLDpmc085y9E
// SIG // uqf03GS9pAHBIAmTeM38vMDJRF1eFpwBBU8iTQIDAQAB
// SIG // o4IB5jCCAeIwEAYJKwYBBAGCNxUBBAMCAQAwHQYDVR0O
// SIG // BBYEFNVjOlyKMZDzQ3t8RhvFM2hahW1VMBkGCSsGAQQB
// SIG // gjcUAgQMHgoAUwB1AGIAQwBBMAsGA1UdDwQEAwIBhjAP
// SIG // BgNVHRMBAf8EBTADAQH/MB8GA1UdIwQYMBaAFNX2VsuP
// SIG // 6KJcYmjRPZSQW9fOmhjEMFYGA1UdHwRPME0wS6BJoEeG
// SIG // RWh0dHA6Ly9jcmwubWljcm9zb2Z0LmNvbS9wa2kvY3Js
// SIG // L3Byb2R1Y3RzL01pY1Jvb0NlckF1dF8yMDEwLTA2LTIz
// SIG // LmNybDBaBggrBgEFBQcBAQROMEwwSgYIKwYBBQUHMAKG
// SIG // Pmh0dHA6Ly93d3cubWljcm9zb2Z0LmNvbS9wa2kvY2Vy
// SIG // dHMvTWljUm9vQ2VyQXV0XzIwMTAtMDYtMjMuY3J0MIGg
// SIG // BgNVHSABAf8EgZUwgZIwgY8GCSsGAQQBgjcuAzCBgTA9
// SIG // BggrBgEFBQcCARYxaHR0cDovL3d3dy5taWNyb3NvZnQu
// SIG // Y29tL1BLSS9kb2NzL0NQUy9kZWZhdWx0Lmh0bTBABggr
// SIG // BgEFBQcCAjA0HjIgHQBMAGUAZwBhAGwAXwBQAG8AbABp
// SIG // AGMAeQBfAFMAdABhAHQAZQBtAGUAbgB0AC4gHTANBgkq
// SIG // hkiG9w0BAQsFAAOCAgEAB+aIUQ3ixuCYP4FxAz2do6Eh
// SIG // b7Prpsz1Mb7PBeKp/vpXbRkws8LFZslq3/Xn8Hi9x6ie
// SIG // JeP5vO1rVFcIK1GCRBL7uVOMzPRgEop2zEBAQZvcXBf/
// SIG // XPleFzWYJFZLdO9CEMivv3/Gf/I3fVo/HPKZeUqRUgCv
// SIG // OA8X9S95gWXZqbVr5MfO9sp6AG9LMEQkIjzP7QOllo9Z
// SIG // Kby2/QThcJ8ySif9Va8v/rbljjO7Yl+a21dA6fHOmWaQ
// SIG // jP9qYn/dxUoLkSbiOewZSnFjnXshbcOco6I8+n99lmqQ
// SIG // eKZt0uGc+R38ONiU9MalCpaGpL2eGq4EQoO4tYCbIjgg
// SIG // tSXlZOz39L9+Y1klD3ouOVd2onGqBooPiRa6YacRy5rY
// SIG // DkeagMXQzafQ732D8OE7cQnfXXSYIghh2rBQHm+98eEA
// SIG // 3+cxB6STOvdlR3jo+KhIq/fecn5ha293qYHLpwmsObvs
// SIG // xsvYgrRyzR30uIUBHoD7G4kqVDmyW9rIDVWZeodzOwjm
// SIG // mC3qjeAzLhIp9cAvVCch98isTtoouLGp25ayp0Kiyc8Z
// SIG // QU3ghvkqmqMRZjDTu3QyS99je/WZii8bxyGvWbWu3EQ8
// SIG // l1Bx16HSxVXjad5XwdHeMMD9zOZN+w2/XU/pnR4ZOC+8
// SIG // z1gFLu8NoFA12u8JJxzVs341Hgi62jbb01+P3nSISRKh
// SIG // ggLOMIICNwIBATCB+KGB0KSBzTCByjELMAkGA1UEBhMC
// SIG // VVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcT
// SIG // B1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jw
// SIG // b3JhdGlvbjElMCMGA1UECxMcTWljcm9zb2Z0IEFtZXJp
// SIG // Y2EgT3BlcmF0aW9uczEmMCQGA1UECxMdVGhhbGVzIFRT
// SIG // UyBFU046N0JGMS1FM0VBLUI4MDgxJTAjBgNVBAMTHE1p
// SIG // Y3Jvc29mdCBUaW1lLVN0YW1wIFNlcnZpY2WiIwoBATAH
// SIG // BgUrDgMCGgMVAKCir3PxP6RCCyVMJSAVoMV61yNeoIGD
// SIG // MIGApH4wfDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldh
// SIG // c2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNV
// SIG // BAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEmMCQGA1UE
// SIG // AxMdTWljcm9zb2Z0IFRpbWUtU3RhbXAgUENBIDIwMTAw
// SIG // DQYJKoZIhvcNAQEFBQACBQDkRslLMCIYDzIwMjEwNTEz
// SIG // MDUzODE5WhgPMjAyMTA1MTQwNTM4MTlaMHcwPQYKKwYB
// SIG // BAGEWQoEATEvMC0wCgIFAORGyUsCAQAwCgIBAAICCs4C
// SIG // Af8wBwIBAAICEjEwCgIFAORIGssCAQAwNgYKKwYBBAGE
// SIG // WQoEAjEoMCYwDAYKKwYBBAGEWQoDAqAKMAgCAQACAweh
// SIG // IKEKMAgCAQACAwGGoDANBgkqhkiG9w0BAQUFAAOBgQCz
// SIG // U3i2gfpKnI5zUKy5xO2gLn7phb5DUz+YGgd2IuIMOHuW
// SIG // Lr/hAZNes23gBOw9PKhVwQ5pB+sFjJthTnN5L0Jt9nS3
// SIG // jS59Knj2zn1kzkGrayCXf91kpQWI/IvENRtgEgAXkJkR
// SIG // dg5DwFDcN3FiDaNiXYnk/DmlATP/njI6WT1l1TGCAw0w
// SIG // ggMJAgEBMIGTMHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQI
// SIG // EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
// SIG // HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAk
// SIG // BgNVBAMTHU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAy
// SIG // MDEwAhMzAAABUcNQ51lsqsanAAAAAAFRMA0GCWCGSAFl
// SIG // AwQCAQUAoIIBSjAaBgkqhkiG9w0BCQMxDQYLKoZIhvcN
// SIG // AQkQAQQwLwYJKoZIhvcNAQkEMSIEIN9ogQgy1aVqbtw8
// SIG // 5k/GfZXHHJyXiex6JqrH9AMlQN31MIH6BgsqhkiG9w0B
// SIG // CRACLzGB6jCB5zCB5DCBvQQgLs1cmYj41sFZBwCmFvv9
// SIG // ScP5tuuUhxsv/t0B9XF65UEwgZgwgYCkfjB8MQswCQYD
// SIG // VQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4G
// SIG // A1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0
// SIG // IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQg
// SIG // VGltZS1TdGFtcCBQQ0EgMjAxMAITMwAAAVHDUOdZbKrG
// SIG // pwAAAAABUTAiBCDSzqKW3BuVjt3DEOIYQ3Vm3g8aVEpJ
// SIG // cjhiDIPEsrMqrTANBgkqhkiG9w0BAQsFAASCAQCP5s45
// SIG // pGlX4dvPE0ewhwqCA/T8tjzHS9HDGKwiiD6R9EnjcgoR
// SIG // 7AM52gG9yuL8OmQRW9tYga21+tq98LQ3YF6huZUwYtIR
// SIG // vd8GCCnfg0a2oX94nrXrnPZXJBWE94UwopzbeYm8SUb0
// SIG // ypfljXo1Ive3Ur7LfZHBboSPTM72Jx8bTfK+Vf1eX7AQ
// SIG // eXGx0Brzwd1LWkN4cDiRl7lRUd6scsCOnOvL2bidZayp
// SIG // TM/cTb/0jxGFfAj3UL8JjqgtOOS+GBqduGiHTDewk9sr
// SIG // eynnC/u4SseI5aHNsbTatrLTzkbHI3RBDYrEJwUi3MtQ
// SIG // hRYRyEsUL7aqrO8ZyBkI0UGhP1Bt
// SIG // End signature block
