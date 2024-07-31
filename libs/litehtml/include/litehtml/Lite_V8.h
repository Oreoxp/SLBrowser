#pragma once
#include <string>
#include "v8.h"
#include "libplatform/libplatform.h"
#include "types.h"
#include "html_tag.h"

namespace litehtml{

class DomInterface {
private:
  std::weak_ptr<element> html_root_;
  
public:
  void setRoot(std::weak_ptr<element>);
  std::shared_ptr<litehtml::element> getElementById(const std::string& id);
  void setInnerText(const std::string& id, const std::string& text);
  void getInnerText(const std::string& id, std::string& text); 
  void setStyle(const std::string& id, const std::string& text);
  void getStyle(const std::string& id, std::string& text);
};

v8::Local<v8::ObjectTemplate> CreateDomTemplate(v8::Isolate* isolate, DomInterface* dom);

class Lite_V8 {
public:
  Lite_V8();
  ~Lite_V8();

  void Init(const std::string& path, int type);
  void ExecuteScript(std::string script);

  void setHtmlRoot(std::weak_ptr<element> ptr);

private:
  std::unique_ptr<v8::Platform> platform;
  v8::Isolate* isolate_;
  v8::Global<v8::Context> context_;
  std::vector<std::string> scripts;
  DomInterface dom_;
  std::weak_ptr<element> root_;
};

}